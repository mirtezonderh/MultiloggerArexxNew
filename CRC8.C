#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include "CRC8.h"



#define poly 0x8C;

uint8_t doCrc(uint8_t l, uint8_t i0, uint8_t i1, uint8_t i2, uint8_t t, uint8_t d0, uint8_t d1, uint8_t c, uint8_t nc){

                       uint8_t decodedPack[7];
                       uint8_t CRC_STATUS = 0;
                       uint8_t BYTE_CTRL;
                       uint8_t BIT_CTRL;
                       uint8_t CRC_CALC;

                       if(l >= 7)
                       {
                           decodedPack[0] = l; // 7
                           decodedPack[1] = i2; // 8
                           decodedPack[2] = i1; // 0
                           decodedPack[3] = i0; // 2
                           decodedPack[4] = t; // 32
                           decodedPack[5] = d0; //10
                           decodedPack[6] = d1; // 176
                       }
                       else
                       {
                           decodedPack[0] = l; //5
                           decodedPack[1] = i1; //
                           decodedPack[2] = i0; //
                           decodedPack[3] = d0; //
                           decodedPack[4] = d1; //
                       }

                       BYTE_CTRL = 0;
                       CRC_CALC = 0;

                       for(BYTE_CTRL = 0; BYTE_CTRL<l; BYTE_CTRL++)
                       {
                           CRC_CALC ^= decodedPack[BYTE_CTRL];

                           for(BIT_CTRL = 0; BIT_CTRL < 8; BIT_CTRL++)
                           {
                               if(CRC_CALC & 1 == 1)
                               {
                                   CRC_CALC>>= 1;
                                   CRC_CALC ^= poly;
                               }
                               else
                               {
                                   CRC_CALC>>= 1;
                               }
                           }
                       }


                       if(CRC_CALC == c)
                       {
                           CRC_CALC = ~CRC_CALC;
                           if(CRC_CALC == nc || l == 5)
                           {
                              CRC_STATUS = 1;
                           }
                           else
                           {
                               CRC_STATUS = 0;
                           }
                       }
                       else{
                           CRC_STATUS = 0;
                       }

return CRC_STATUS;
}
