#include <string.h>
#include <time.h>

#include "timex_utils.h"

float hmsmsToSecond(uint8_t h, uint8_t m, uint8_t s, uint8_t ms);

void responseToSample(unsigned char *response, struct sample *smpl) {
  struct rawSample raw;
  memcpy(&raw, response, sizeof(struct rawSample));

  smpl->hr = raw.hr;
  smpl->hrStatus = raw.hrStatus;
  
  smpl->gpsLat = (float)raw.gpsLat / 1000000.0;
  smpl->gpsLong = (float)raw.gpsLong / 1000000.0;
  smpl->gpsAlt = raw.gpsAlt;
  smpl->gpsSpeed = (float)raw.gpsSpeed * 0.002777781;
  smpl->gpsStatus = raw.gpsStatus;
  smpl->compass = raw.compass;
  
  smpl->timeDiff = (float)raw.timeDiff / 100.0;
  smpl->distanceDiff = (float)raw.distanceDiff / 100.0;
  smpl->dist = raw.dist;
}

void responseToLap(unsigned char *response, struct lap *l) {
  struct rawLap rLap;
  memcpy(&rLap, response, sizeof(struct rawLap));

  l->lapNumber = rLap.lapNumber;
  l->dist = rLap.dist;
  l->altMin = rLap.altMin;
  l->altMax = rLap.altMax;
  l->endTime = hmsmsToSecond(rLap.endH, rLap.endM, rLap.endS, rLap.endMS);
  l->duration = hmsmsToSecond(rLap.dH, rLap.dM, rLap.dS, rLap.dMS);
}

void responseToOverview(unsigned char *response, struct fileOverview *overview) {
  memset(overview, 0, sizeof(struct fileOverview));

  struct tm tmpDate;

  tmpDate.tm_year = 100 + response[0];
  tmpDate.tm_mon = response[1] - 1;
  tmpDate.tm_mday = response[2];
  tmpDate.tm_hour = response[3];
  tmpDate.tm_min = response[4];
  tmpDate.tm_sec = response[5];
  overview->start = mktime(&tmpDate);

  memcpy(&(overview->lapsCount), response + 6, sizeof(uint16_t));
  
  overview->duration = response[8] * 60 * 60 + response[9] * 60 + response[10];

  memcpy(&(overview->ascent), response + 40, sizeof(uint16_t));

  memcpy(&(overview->descent), response + 42, sizeof(uint16_t));


  memcpy(&(overview->addr), response + 70, sizeof(uint16_t));

  overview->fileNumber = response[72];
}

float hmsmsToSecond(uint8_t h, uint8_t m, uint8_t s, uint8_t ms) {
  return h * 60 * 60 + m * 60 + s + (float)ms / 100;
}
