#ifndef SDCARD_H_INCLUDED
#define SDCARD_H_INCLUDED

#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include <stdint.h>


/* prototypes */

void createTxtFileOnSD(uint32_t baseId);
void writeDataOnSD(
                   uint32_t baseId,
                   uint32_t sensorId,
                   float data
                   );

#endif
