#include <stdint.h>
#include <time.h>

#ifndef TIMEX_TYPES_H 
#define TIMEX_TYPES_H

#pragma pack(push, 1)
struct rawSample {
  uint8_t hr;
  uint8_t hrStatus;
  char unknown0;
  uint32_t gpsLat;
  uint32_t gpsLong;
  uint16_t gpsAlt;
  uint16_t gpsSpeed;
  uint16_t compass;
  uint16_t distanceDiff;
  uint8_t timeDiff;
  char unknown2[3];
  uint8_t gpsStatus;
  char unknown3[21];
  uint16_t dist;
  char unknown4; 
};
#pragma pack(pop)

#pragma pack(push, 1)
struct rawLap {
  uint8_t endH;
  uint8_t endM;
  uint8_t endS;
  uint8_t endMS;
  uint8_t dH;
  uint8_t dM;
  uint8_t dS;
  uint8_t dMS;
  char unknown0[4];
  uint16_t dist;
  char unknown1[2];
  uint8_t lapNumber;  
  char unknown2[23];
  uint16_t altMin;
  uint16_t altMax;
  char unknown3[20];
};
#pragma pack(pop)

struct sample {
  uint8_t hr;
  uint8_t hrStatus;
  float gpsLat;
  float gpsLong;
  uint16_t gpsAlt;
  float gpsSpeed;
  uint16_t compass;
  float distanceDiff;
  float timeDiff;
  uint8_t gpsStatus;
  uint16_t dist;
};

struct lap {
  int lapNumber;
  float duration;
  float endTime;
  int dist;
  int altMin;
  int altMax;
  int samplesCount;
  struct sample *samples;
};

struct fileOverview {
  time_t start;
  time_t duration;
  uint16_t ascent;
  uint16_t descent;
  uint16_t addr;
  uint8_t fileNumber;
  uint16_t lapsCount;
  struct lap *laps;
};

#endif
