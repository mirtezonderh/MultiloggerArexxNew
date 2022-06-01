#include <GetAndSetData.h>
#include <stdlib.h>
#include <stdio.h>
#include <xdc/std.h>
#include <stdint.h>
#include <String.h>

String dataType;
String dataUnit;
uint32_t sensId;

void setType(String type)
{
    dataType = type;
}

String getType()
{
    return dataType;
}
void setUnit(String unit){
    dataUnit = unit;
}
String getUnit(){

    return dataUnit;
}

void setId(uint32_t ID)
{
    sensId = ID;
}

uint32_t getId()
{

    return sensId;
}
