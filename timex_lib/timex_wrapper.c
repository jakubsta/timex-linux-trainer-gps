#include <python2.7/Python.h>

#include "timex.h"
#include "timex_types.h"

PyObject* convertFilesOverview(struct fileOverview *files, int filesCount);
PyObject* convertSamples(struct sample *samples, int samplesCount, int *startTime, float *distance);
PyObject* convertLaps(struct lap *laps, int lapsCount, int startTime);

static PyObject *timex_wrapper(PyObject *self, PyObject *args) {
  char *devicePath;

  // parse arguments
  if (!PyArg_ParseTuple(args, "s", &devicePath)) {
    return NULL;
  }

  struct fileOverview *files;
  int filesCount;

  // run the actual function
  readTimex(
      devicePath, 
      &files, 
      &filesCount);

  // build the result.
  PyObject *pyFiles = convertFilesOverview(files, filesCount);

  int i;
  for(i = 0; i < filesCount; i++) {
    free(files[i].laps[0].samples);
    free(files[i].laps);
  }
  free(files);

  return pyFiles;
}

static PyMethodDef TimexMethods[] = {
 { "read", timex_wrapper, METH_VARARGS, "Read watch content" },
 { NULL, NULL, 0, NULL }
};

static PyObject *TimexError;

PyMODINIT_FUNC inittimex(void) {
    PyObject *m;

    m = Py_InitModule("timex", TimexMethods);
    if (m == NULL)
        return;

    TimexError = PyErr_NewException("timex.error", NULL, NULL);
    Py_INCREF(TimexError);
    PyModule_AddObject(m, "error", TimexError);
}

PyObject* convertSamples(struct sample *samples, int samplesCount, int *startTime, float *distance) {
  PyObject *pySamples = PyList_New(samplesCount);
  int i;
  for(i = 0; i < samplesCount; i++) {
    *startTime += (int)samples[i].timeDiff;
    *distance += samples[i].distanceDiff;

    PyObject *sample = Py_BuildValue(
        "{s:B, s:f, s:f, s:H, s:f, s:i, s:f}",
        "hr", samples[i].hr,
        "lat", samples[i].gpsLat,
        "lng", samples[i].gpsLong,
        "alt", samples[i].gpsAlt,
        "speed", samples[i].gpsSpeed,
        "time", *startTime,
        "dist", *distance
        );
    PyList_SET_ITEM(pySamples, i, sample);
  }

  return pySamples;
}

PyObject* convertLaps(struct lap *laps, int lapsCount, int startTime) {
  PyObject *pyLaps = PyList_New(lapsCount);
  int i;
  float dist = 0;

  for(i = 0; i < lapsCount; i++) {
    PyObject *samples = convertSamples(
        laps[i].samples, 
        laps[i].samplesCount,
        &startTime,
        &dist);

    PyObject *lap = Py_BuildValue(
      "{s:f, s:f, s:i, s:i, s:i, s:i, s:O}",
      "endTime", laps[i].endTime,
      "duration", laps[i].duration,
      "dist", laps[i].dist,
      "altMin", laps[i].altMin,
      "altMax", laps[i].altMax,
      "lapsNumber", laps[i].lapNumber,
      "samples", samples
      );
    PyList_SET_ITEM(pyLaps, i, lap);
  }
  
  return pyLaps;
}

PyObject* convertFilesOverview(struct fileOverview *files, int filesCount) {
  PyObject *pyFiles = PyList_New(filesCount);
  int i;
  for(i = 0; i < filesCount; i++) {
    PyObject *laps = convertLaps(
        files[i].laps, 
        files[i].lapsCount, 
        files[i].start);

    PyObject *file = Py_BuildValue(
        "{s:i, s:i, s:H, s:H, s:B, s:O}",
        "start", files[i].start,
        "duration", files[i].duration,
        "ascent", files[i].ascent,
        "descent", files[i].descent,
        "fileNumber", files[i].fileNumber,
        "laps", laps
        );
    PyList_SET_ITEM(pyFiles, i, file);
  }

  return pyFiles;
}
