#include "UnpackPayload.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xdc/std.h>
#include <stdint.h>
#include <CRC8.h>
#include <math.h>
#include <GetAndSetData.h>

uint8_t CRC_OK_NOK;
uint8_t LENGTH;
uint8_t ID0;
uint8_t ID1;
uint8_t ID2;
uint8_t TYPE;
uint8_t DATA0;
uint8_t DATA1;
uint8_t CRC;
uint8_t nCRC;
uint16_t DATA_FULL;
uint16_t temp_id5;
uint32_t temp_id;
uint32_t ID_FULL;



uint8_t unpackFiveCRC(uint8_t packetbuff[])
{
    CRC_OK_NOK = 0;
    ID0 = 0;
    ID1 = 0;
    ID2 = 0;
    TYPE = 0;
    DATA0 = 0;
    DATA1 = 0;
    CRC = 0;
    LENGTH = 5;

    ID0 |= packetbuff[4];
    ID0 <<= 4;
    ID0 |= packetbuff[5];

    ID1 |= packetbuff[2];
    ID1 <<= 4;
    ID1 |= packetbuff[3];

    DATA0 |= packetbuff[6];
    DATA0 <<= 4;
    DATA0 |= packetbuff[7];

    DATA1 |= packetbuff[8];
    DATA1 <<= 4;
    DATA1 |= packetbuff[9];

    CRC |= packetbuff[10];
    CRC <<= 4;
    CRC |= packetbuff[11];

    CRC_OK_NOK = doCrc(LENGTH, ID0, ID1, 0, 0, DATA0, DATA1, CRC, 0);

    return CRC_OK_NOK;
}


double calculateFive()
{

    double DataCalc = 0;
    DATA_FULL = 0;
    ID_FULL = 0;
    temp_id5 = 0;


    DATA_FULL |= DATA0;
    DATA_FULL <<= 8;
    DATA_FULL |= DATA1;
    DataCalc = DATA_FULL;

    ID_FULL |= ID0;
    ID_FULL <<= 8;
    ID_FULL |= ID1;

    /* Calculation of data is same for humidity and temperature */
    DataCalc /= 100;
    DataCalc *= 0.78125;

    /* Check last bit of ID */
    temp_id5 = ID_FULL;
    temp_id5 <<= 15;
    temp_id5 >>= 15;

    /* If last bit is 1 then data is temperature */
    if (temp_id5 == 1)
    {

        setType("Temp");
        setUnit("C");


    }

    /* If last bit is 0 then data is humidity */
    else
    {

        setType("RH");
        setUnit("%");

    }

    setId(ID_FULL);
    return DataCalc;


}

uint8_t unpackSevenCRC(uint8_t packetbuff[])
{
    LENGTH = 7;
    CRC_OK_NOK = 0;
    ID0 = 0;
    ID1 = 0;
    ID2 = 0;
    TYPE = 0;
    DATA0 = 0;
    DATA1 = 0;
    CRC = 0;
    nCRC = 0;


    ID0 |= packetbuff[6];
    ID0 <<= 4;
    ID0 |= packetbuff[7];

    ID1 |= packetbuff[4];
    ID1 <<= 4;
    ID1 |= packetbuff[5];

    ID2 |= packetbuff[2];
    ID2 <<= 4;
    ID2 |= packetbuff[3];

    TYPE |= packetbuff[8];
    TYPE <<= 4;
    TYPE |= packetbuff[9];

    DATA0 |= packetbuff[10];
    DATA0 <<= 4;
    DATA0 |= packetbuff[11];

    DATA1 |= packetbuff[12];
    DATA1 <<= 4;
    DATA1 |= packetbuff[13];

    CRC |= packetbuff[14];
    CRC <<= 4;
    CRC |= packetbuff[15];

    nCRC |= packetbuff[16];
    nCRC <<= 4;
    nCRC |= packetbuff[17];

    CRC_OK_NOK = doCrc(LENGTH, ID0, ID1, ID2, TYPE, DATA0, DATA1, CRC, nCRC);

     return CRC_OK_NOK;

}

double calculateSeven()
{

    String type;
    String unit;
    double DataCalc = 0;
    double degC = 0;
    double hum = 0;
    double vbat = 0;
    DATA_FULL = 0;
    ID_FULL = 0;
    temp_id = 0;

    DATA_FULL |= DATA0;
    DATA_FULL <<= 8;
    DATA_FULL |= DATA1;

    ID_FULL |= ID0;
    ID_FULL <<= 8;
    ID_FULL |= ID1;
    ID_FULL <<= 8;
    ID_FULL |= ID2;

    switch (TYPE)
    {
        case 32: /*20, CHECKED: TC77 OR MCP9808 temperature */

            DATA_FULL <<= 3;
            degC = DATA_FULL;
            degC /= 100;
            degC *= 0.09765625;
            DataCalc = degC;

            type = "RH";
            unit = "%";

            break;

        case 0x48: /*30, NOT CHECKED: T6613 CO2*/

            // not used
            break;

        case 64: /*40, CHECKED: SHT10 humidity and temperature */

            /* Shift to get last bit of ID */
            temp_id = 0;
            temp_id = ID_FULL;
            temp_id <<= 31;
            temp_id >>= 31;

            /* if last bit of ID is 1, data is humidity */
            if (temp_id == 1)
                {
                    float tempHum = DATA_FULL * DATA_FULL;
                    tempHum *= 1.5955E-6;
                    hum = DATA_FULL;
                    hum *= 0.0367;
                    hum -= 2.0468;
                    hum -= tempHum;
                    DataCalc = hum;
                    type = "RH";
                    unit = "%";

                }

            /* if last bit of ID is 0, data is humidity */
            else
                {
                    degC = DATA_FULL;
                    degC *= 0.01;
                    degC -= 39.6;
                    DataCalc = degC;
                    type = "Temp";
                    unit = "C";

                }
                break;


        case 80: /*50, NOT CHECKED, HDS1080 hum/temp */

            /* if humidity */
            hum = DATA_FULL;
            hum *= 0.152587890625;
            /* if temperature */
            degC = DATA_FULL;
            degC *= 0.25177001953125;
            degC -= 4000;
                break;

        case 224:  /*E0 Temperature of pt100*/

            degC = DATA_FULL;
            degC /= 100;
            degC *= 0.390625;
            DataCalc = degC;
            type = "Temp";
            unit = "C";

                break;

        case 240: //F0, Battery voltage

            vbat = 2097.152;
            vbat /= DATA_FULL;
            DataCalc = vbat;
            type = "Volt";
            unit = "V";

                break;

        case 112: //0x70, SHT40 temp
            DataCalc = DATA_FULL;
            DataCalc = -45 + (175* (DataCalc/65535));
            type = "Temp";
            unit = "C";

            break;
        case 113: //0x71, SHT40 Hum
            DataCalc = DATA_FULL;
            DataCalc = -6 + (125* (DataCalc/65535));
            type = "RH";
            unit = "%";

            break;

        case 128:// 0x80, SCD41 TEMP
            DataCalc = DATA_FULL;
            DataCalc = -45 + (175* (DataCalc/65535));
            type = "Temp";
            unit = "C";

            break;
        case 129: //0x81, SCD41 HUM
            DataCalc = DATA_FULL;
            DataCalc = 100*(DataCalc/65535);
            type = "RH";
            unit = "%";
            break;
        case 130: //0x82 SCD41 CO2
            DataCalc = DATA_FULL;
            type = "CO2";
            unit = "PPM";
            break;



        default:
            /* Uknown type*/
            DataCalc = 101;
            type = "Unknown";
            unit = "Unknown";
                break;

    }
    if(DataCalc < 0){
        DataCalc = 0;
    }
    setType(type);
    setUnit(unit);
    setId(ID_FULL);
    return DataCalc;

}

