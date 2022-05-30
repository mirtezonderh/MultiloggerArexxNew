#ifndef UNPACK_PAYLOAD_H_INCLUDED
#define UNPACK_PAYLOAD_H_INCLUDED

#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include <stdint.h>


uint8_t unpackFiveCRC(uint8_t packetbuff[]);
uint8_t unpackSevenCRC(uint8_t packetbuff[]);
double calculateFive();
double calculateSeven();
uint32_t getIdTemp();

#endif
