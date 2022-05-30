#ifndef GETANDSETDATA_H_INCLUDED
#define GETANDSETDATA_H_INCLUDED

#include <stdlib.h>
#include<stdio.h>
#include <xdc/std.h>
#include <stdint.h>

uint32_t setId();
uint32_t getId();
void setType(String type);
String getType();
void setUnit(String unit);
String getUnit();
char setTimeStamp();
char getTimeStamp();
char setFileName();
char getFileName();

#endif
