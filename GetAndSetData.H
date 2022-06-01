#ifndef GETANDSETDATA_H_INCLUDED
#define GETANDSETDATA_H_INCLUDED

#include <stdlib.h>
#include<stdio.h>
#include <xdc/std.h>
#include <stdint.h>

void setId(uint32_t ID);
uint32_t getId();
void setType(String type);
String getType();
void setUnit(String unit);
String getUnit();

#endif
