#include "timex_types.h"

#ifndef TIMEX_UTILS_H 
#define TIMEX_UTILS_H

void responseToSample(unsigned char *response, struct sample *smpl);
void responseToLap(unsigned char *response, struct lap *l);
void responseToOverview(unsigned char *response, struct fileOverview *overview);

#endif
