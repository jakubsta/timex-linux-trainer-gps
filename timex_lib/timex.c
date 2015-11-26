#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "timex.h"
#include "timex_types.h"
#include "timex_const.h"
#include "timex_utils.h"

#define BUFFER_SIZE 4096

int initAndOpenDev(char *device); 
int request(int ttyFd, unsigned char *instruction, int instructionLen, unsigned char **response);
int combineArrays(struct sample **dest, int count, struct sample **source, int sourceCount);
int parseFileSamples(unsigned char *response, int responseLen, struct sample **samplesList);
int parseFileLaps(unsigned char *response, int responseLen, struct lap **laps);
int parseFilesOverView(unsigned char *response, int responseLen, struct fileOverview **filesList);
int forceFetchingFile(int ttyFd, struct fileOverview *overview);
void splitSamplesIntoLaps(struct fileOverview *overview, struct sample *samples, int samplesCount);
int readFile(int ttyFd, struct fileOverview *overview);

void readTimex(char *device, struct fileOverview **filesList, int *fileCount) {
  int ttyFd = initAndOpenDev(device);
  int responseLen;
  unsigned char *res = NULL;

  /* Downloading files overview */
  responseLen = request(ttyFd, message4, message4Len, &res);
  *fileCount = parseFilesOverView(res, responseLen, filesList);
  free(res);

  /* Downloading files */
  int i;
  for(i=0; i<*fileCount; i++) {
    readFile(ttyFd, (&(*filesList)[i]));
  }
  
  close(ttyFd);
}

int forceFetchingFile(int ttyFd, struct fileOverview *overview) {
  int laps = 0;
  unsigned char *res = NULL;
  int responseLen = 0;
  int fileReqLen = 9;
  unsigned char *fileReq = (unsigned char*)malloc(fileReqLen*sizeof(unsigned char));
  memset(fileReq, 0, fileReqLen);
  memcpy(fileReq, FETCH_FILE_PREFIX, FETCH_FILE_PREFIX_LEN);
  fileReq[6] = overview->addr;
  fileReq[8] = 0x84 + overview->addr;

  responseLen = request(ttyFd, fileReq, fileReqLen, &res);
  
  if(responseLen != 0) {
    printf("File %d addr 0x%X\n", overview->fileNumber, fileReq[8]);
    laps = parseFileLaps(res, responseLen, &(overview->laps));
    free(res);
    return laps;
  }
  
  int i;
  for(i=0; i<=0xff; i++) {
    free(res);
    fileReq[8] = i;
    responseLen = request(ttyFd, fileReq, fileReqLen, &res);
    if(responseLen != 0) {
      printf("File %d addr 0x%X\n", overview->fileNumber, fileReq[8]);
      laps = parseFileLaps(res, responseLen, &(overview->laps));
      break;
    }
  }

  free(res);
  return laps;
}

int readFile(int ttyFd, struct fileOverview *overview) {
  unsigned char *res = NULL;
  struct sample *fileSamples = NULL;
  int responseLen, samplesCount = 0;

  forceFetchingFile(ttyFd, overview);

  do {
    free(res);
    responseLen = request(ttyFd, continueFetching, continueFetchingLen, &res);

    struct sample *samples = NULL;
    int readedSamples = parseFileSamples(res, responseLen, &samples);

    samplesCount = combineArrays(
        &fileSamples, 
        samplesCount, 
        &samples, 
        readedSamples);
  } while(memcmp(res, timexEOF, timexEOFLen) != 0);
  free(res);

  splitSamplesIntoLaps(overview, fileSamples, samplesCount);

  return samplesCount;
}

int combineArrays(struct sample **dest, int count, struct sample **source, int sourceCount) {
  if(count == 0) {
    *dest = *source;
    return sourceCount;
  } else if(sourceCount == 0) {
    return count;
  }
  
  *dest = (struct sample*)realloc(
      *dest, 
      (count + sourceCount) * sizeof(struct sample));

  memcpy(
      &((*dest)[count]), 
      *source, 
      sourceCount * sizeof(struct sample));

  return count + sourceCount;
}

int request(int ttyFd, unsigned char *instruction, int instructionLen, unsigned char **response) {
  int writed = 0;
  do {
    instruction += writed;
    writed += write(ttyFd, instruction, instructionLen - writed);
  } while(writed != instructionLen);

  *response = (unsigned char*)malloc(BUFFER_SIZE * sizeof(unsigned char));

  int readed = 0;
  int responseLen = 0;
  do {
    readed = read(ttyFd, *response + responseLen, BUFFER_SIZE - responseLen);
    responseLen += readed > 0 ? readed : 0;
  } while(readed > 0);

  return responseLen;
}

int initAndOpenDev(char *device) {
	struct termios tio;
	int ttyFd;
	
  memset(&tio, 0, sizeof(tio));
	tio.c_iflag = 0;
	tio.c_oflag = 0;

  /* Character size mask.  Values are CS5, CS6, CS7, or CS8. */
  /* Enable receiver. */
  /* Ignore modem control lines. */ 
	/* CRTSCTS -- disable hardware control */
	tio.c_cflag = CS8 | CREAD | CLOCAL; 

  tio.c_lflag = 0;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 2;

  /* O_NOCTTY */
  /* If pathname refers to a terminal device—see tty(4)—it will  not  become */
  /* the  process's  controlling  terminal even if the process does not have */
  /* one. */
	ttyFd = open(device, O_RDWR | O_NOCTTY);
  if(ttyFd < 0) {
    perror("open device");
    exit(EXIT_FAILURE);
  }

	cfsetospeed(&tio, B9600);
	cfsetispeed(&tio, B9600);
	tcsetattr(ttyFd, TCSANOW, &tio);

  return ttyFd;
}

void splitSamplesIntoLaps(struct fileOverview *overview, struct sample *samples, int samplesCount) {
  overview->laps[0].samples = samples;

  if(overview->lapsCount == 1) {
    overview->laps[0].samplesCount = samplesCount;
    return;
  }

  int i, j = 0;
  float tmpTime = 0.0;
  for(i = 0; i < overview->lapsCount; i++) {
    float maxTime = overview->laps[i].endTime;
    int startSample = j; 
    int lapSamplesCount;
    for(lapSamplesCount = 0; j < samplesCount; lapSamplesCount++, j++) {
      tmpTime += samples[j].timeDiff;
      if(maxTime < tmpTime) {
        break;
      }
    }
    overview->laps[i].samples = &(samples[startSample]);
    overview->laps[i].samplesCount = lapSamplesCount;
  }
}

int parseFileSamples(unsigned char *response, int responseLen, struct sample **samplesList) {
  /* skip preambulemble */ 
  int samplesCount = 0;
  int readed = 80;
  response += readed;

  int currentMax = 42;
  struct sample *samples;
  samples = (struct sample*)calloc(currentMax, sizeof(struct sample));

  while(readed + (int)sizeof(struct rawSample) <= responseLen) {
    if(samplesCount == currentMax) {
      currentMax += 5;
      samples = (struct sample*)realloc(
          samples, 
          currentMax * sizeof(struct sample));
    }
    responseToSample(response, &(samples[samplesCount]));

    response += sizeof(struct rawSample);
    readed += sizeof(struct rawSample);
    ++samplesCount;
  }

  samples = (struct sample*)realloc(
      samples, 
      samplesCount * sizeof(struct sample));

  *samplesList = samples;
  return samplesCount;
}

int parseFileLaps(unsigned char *response, int responseLen, struct lap **laps) {
  /* skip preambulemble */ 
  int lapsCount = 0;
  int readed = 79;
  response += readed;

  int currentMax = 5;
  struct lap *tmpLaps;
  tmpLaps = (struct lap*)calloc(currentMax, sizeof(struct lap));

  while(readed + (int)sizeof(struct rawLap) <= responseLen) {
    if(lapsCount == currentMax) {
      currentMax += 5;
      tmpLaps = (struct lap*)realloc(tmpLaps, currentMax * sizeof(struct lap));
    }
    responseToLap(response, &(tmpLaps[lapsCount]));

    response += sizeof(struct rawLap);
    readed  += sizeof(struct rawLap);
    ++lapsCount;
  }

  tmpLaps = (struct lap*)realloc(tmpLaps, lapsCount * sizeof(struct lap));

  *laps = tmpLaps;
  return lapsCount;
}

int parseFilesOverView(unsigned char *response, int responseLen, struct fileOverview **filesList) {
  /* skip preambulemble */ 
  int fileCount = 0;
  int readed = 3;
  response += readed;

  int currentMax = 5;
  struct fileOverview *filesOverview = NULL;
  filesOverview = (struct fileOverview*)calloc(currentMax, sizeof(struct fileOverview));

  while(readed + fileOverviewSize <= responseLen) {
    if(fileCount == currentMax) {
      currentMax += 5;
      filesOverview = (struct fileOverview*)realloc(
          filesOverview, 
          currentMax * sizeof(struct fileOverview));
    }
    responseToOverview(response, &(filesOverview[fileCount]));

    response += fileOverviewSize;
    readed += fileOverviewSize;
    ++fileCount;
  }
  
  filesOverview = (struct fileOverview*)realloc(
      filesOverview,
      fileCount * sizeof(struct fileOverview));

  *filesList = filesOverview;
  return fileCount;
}


