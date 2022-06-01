#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include <stdint.h>
#include <SDcard.h>
#include <time.h>
#include "ErrorBlinkyCases.H"
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


/*This function can be ignored. The FatFS lib requires the user to implement this function themselves.*/

int32_t fatfs_getFatTime(void)
{
    time_t seconds;
    uint32_t fatTime;
    struct tm *pTime;


    seconds = time(NULL);

    pTime = localtime(&seconds);

    fatTime = ((uint32_t) (pTime->tm_year - 80) << 25)
            | ((uint32_t) (pTime->tm_mon) << 21)
            | ((uint32_t) (pTime->tm_mday) << 16)
            | ((uint32_t) (pTime->tm_hour) << 11)
            | ((uint32_t) (pTime->tm_min) << 5)
            | ((uint32_t) (pTime->tm_sec) >> 1);

    return ((int32_t) fatTime);
}

/* SDFatFS configurations */
SDFatFS_Object myObj;
SDFatFS_Config SDFatFS_config[] = { { &myObj }, NULL };
uint_least8_t SDFatFS_count = sizeof(SDFatFS_config) / sizeof(SDFatFS_config[0])- 1;





void createTxtFileOnSD(uint32_t baseId)
{
      SDFatFS_Handle sd_handle;
      SDFatFS_init();

      /* INIT spi with SD card libs */
      sd_handle = SDFatFS_open(Board_SD0, 0);
      if (sd_handle == NULL)
      {
          while (1)
              ;
          errorBlinkCase(1);
      }

      fr = f_open(&fsrc, "Log.TXT", FA_OPEN_EXISTING | FA_WRITE);

      switch (fr)
      {
      case FR_NO_FILE:
          fr = f_open(&fsrc, "Log.TXT", FA_CREATE_NEW | FA_WRITE);
          break;

      default:
          /* is ok */
          break;
      }
      if (fr != 0)
      {
          while (1)
              ;
          errorBlinkCase(1);
      }

      /* Close open files */
      fr = f_close(&fsrc);


}


void writeDataOnSD(uint32_t baseId, uint32_t sensorId, float data)
{



}

