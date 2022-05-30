#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include <stdint.h>
#include <SDcard.h>
#include <time.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/SD.h>
#include <ti/display/Display.h>
#include <third_party/fatfs/ff.h>
#include <third_party/fatfs/diskio.h>
#include <third_party/fatfs/ffcio.h>
#include <third_party/fatfs/ffconf.h>
#include "ti/drivers/SDFatFS.h"
/* Example/Board Header files */
#include "Board.h"

FIL fsrc; /* File objects */
FRESULT fr; /* FatFs function common result code */
UINT bw; /* File read/write count */



int32_t fatfs_getFatTime(void)
{
    time_t seconds;
    uint32_t fatTime;
    struct tm *pTime;

    /*
     *  TI time() returns seconds elapsed since 1900, while other tools
     *  return seconds from 1970.  However, both TI and GNU localtime()
     *  sets tm tm_year to number of years since 1900.
     */
    seconds = time(NULL);

    pTime = localtime(&seconds);

    /*
     *  localtime() sets pTime->tm_year to number of years
     *  since 1900, so subtract 80 from tm_year to get FAT time
     *  offset from 1980.
     */
    fatTime = ((uint32_t) (pTime->tm_year - 80) << 25)
            | ((uint32_t) (pTime->tm_mon) << 21)
            | ((uint32_t) (pTime->tm_mday) << 16)
            | ((uint32_t) (pTime->tm_hour) << 11)
            | ((uint32_t) (pTime->tm_min) << 5)
            | ((uint32_t) (pTime->tm_sec) >> 1);

    return ((int32_t) fatTime);
}

SDFatFS_Object myObj;
SDFatFS_Config SDFatFS_config[] = { { &myObj },
NULL };
uint_least8_t SDFatFS_count = sizeof(SDFatFS_config) / sizeof(SDFatFS_config[0])
        - 1;

void createTxtFileOnSD(uint32_t baseId)
{
    SD_init();
    SDFatFS_Handle sd_handle;
    SDFatFS_init();

    sd_handle = SDFatFS_open(Board_SD0, 0);
    if (sd_handle == NULL)
    {
        while (1);
    }
    char TXTNAME[15];
    uint32_t TXTID = baseId;
    snprintf(TXTNAME, 15, "%d.txt", TXTID);


    fr = f_open(&fsrc, TXTNAME, FA_OPEN_EXISTING | FA_WRITE);
    switch (fr)
    {
    case FR_NO_FILE:
        fr = f_open(&fsrc, TXTNAME, FA_CREATE_NEW | FA_WRITE);

    default:
        break;
    }

    if (fr != 0)
    {
        while (1);

    }
    fr = f_close(&fsrc);


}


void writeDataOnSD(uint32_t baseId, uint32_t sensorId, float data)
{

    char buffer[50];
    int cx;
    int i = 0;
    char TXTNAME = "12345.txt";

    fr = f_open(&fsrc, "test.txt", FA_OPEN_APPEND | FA_WRITE);

    cx = snprintf (buffer, sizeof(buffer), "\{\"ID\":\"%d\"\,\"Data\":%0.2f\}", sensorId, data);



    while(i<2)
    {
        fr = f_write(&fsrc, buffer, cx, &bw);
        i++;
    }
    /* if writing nOk */
    if(fr != 0)
        {
            while(1);
        }

    /* Try to sync */
    fr = f_sync(&fsrc);
    if(fr != 0)
        {
            while(1);
        }

    fr = f_close(&fsrc);

}

