
/***** Includes *****/

/* Standard C Libraries */
#include<stdio.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <String.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/SD.h>
#include <ti/drivers/UART.h>
#include <ti/sysbios/hal/Seconds.h>

/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)

/* FatFS SD card libraries*/
#include <third_party/fatfs/ff.h>
#include <third_party/fatfs/diskio.h>
#include <third_party/fatfs/ffcio.h>
#include <third_party/fatfs/ffconf.h>
#include "ti/drivers/SDFatFS.h"
#include <ti/drivers/SD.h>

/* Board Header files */
#include "Board.h"

/* Application Header files */
#include "RFQueue.h"
#include "smartrf_settings/smartrf_settings.h"

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Task.h>


/* Custom header files */
#include <SDcard.h>
#include <CRC8.h>
#include <Manchester.h>
#include <UnpackPayload.h>
#include <GetAndSetData.H>
#include <EpochSet.H>
#include <ErrorBlinkyCases.H>

/* Fatfs defines */

FIL fsrc; /* File objects */
FRESULT fr; /* FatFs function common result code */
UINT bw; /* File read/write count */

/* RF Defines */

/* Packet RX Configuration */
#define DATA_ENTRY_HEADER_SIZE 9  /* Constant header size of a Generic Data Entry */
#define MAX_LENGTH             19 /* Max length byte the radio will accept */
#define NUM_DATA_ENTRIES       2  /* NOTE: Only two data entries supported at the moment */
#define NUM_APPENDED_BYTES     2  /* The Data Entries data field will contain:
                                   * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
                                   * Max 30 payload bytes
                                   * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */

/***** Prototypes *****/
static void callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);

/***** Variable declarations *****/
static RF_Object rfObject;
static RF_Handle rfHandle;

/* Buffer which contains all Data Entries for receiving data.
 * Pragmas are needed to make sure this buffer is 4 byte aligned (requirement from the RF Core) */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN (rxDataEntryBuffer, 4);
static uint8_t rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(
        NUM_DATA_ENTRIES, MAX_LENGTH, NUM_APPENDED_BYTES)];

#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 4
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__GNUC__)
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)]
                                                  __attribute__((aligned(4)));
#else
#error This compiler is not supported.
#endif

/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;
static rfc_dataEntryGeneral_t *currentDataEntry;
static uint8_t packetLength;
static uint8_t *packetDataPointer;

/* packet to store received bytes in*/
static uint8_t packet[MAX_LENGTH + NUM_APPENDED_BYTES - 1]; /* The length byte is stored in a separate variable */

/* Custom raw and decoded payloads */
uint8_t payload[19];
uint8_t decoded[MAX_LENGTH];

/* RX Semaphore */
static Semaphore_Struct rxSemaphore;
static Semaphore_Handle rxSemaphoreHandle;
/* END of RF definitions */
static time_t t1;



void* mainThread(void *arg0)
{

    RF_Params rfParams;
    RF_Params_init(&rfParams);

    /* Initialize RX semaphore */
    Semaphore_construct(&rxSemaphore, 0, NULL);
    rxSemaphoreHandle = Semaphore_handle(&rxSemaphore);

    if (RFQueue_defineQueue(&dataQueue, rxDataEntryBuffer,
                            sizeof(rxDataEntryBuffer),
                            NUM_DATA_ENTRIES,
                            MAX_LENGTH + NUM_APPENDED_BYTES))
    {
        /* Failed to allocate space for all data entries */
        while (1)
        {
            errorBlinkCase(3);
        }
    }

    /* Modify CMD_PROP_RX command for application needs */

    /* Set the Data Entity queue for received data */
    RF_cmdPropRx.pQueue = &dataQueue;
    /* Discard ignored packets from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 1;
    /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRx.maxPktLen = MAX_LENGTH;
    RF_cmdPropRx.pktConf.bRepeatOk = 1;
    RF_cmdPropRx.pktConf.bRepeatNok = 1;

    /* Request access to the radio */
#if defined(DeviceFamily_CC26X0R2)
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioSetup, &rfParams);
#else
    rfHandle = RF_open(&rfObject, &RF_prop,
                       (RF_RadioSetup*) &RF_cmdPropRadioDivSetup, &rfParams);
#endif// DeviceFamily_CC26X0R2


   // uint32_t epoch = getEpoch();
   // Seconds_set(epoch);
    Seconds_set(1654100815);

    createTxtFileOnSD();

    GPIO_write(Board_GPIO_RLED,0);
    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*) &RF_cmdFs, RF_PriorityNormal, NULL, 0);

    /* Enter RX mode and stay forever in RX */
    RF_postCmd(rfHandle, (RF_Op*) &RF_cmdPropRx, RF_PriorityNormal, &callback,
    RF_EventRxEntryDone);


    while (1)
    {

        Semaphore_pend(rxSemaphoreHandle, BIOS_WAIT_FOREVER);

        uint32_t idReturned = 0;
        double calculatedData = 0;
        uint8_t resultCrc = 0;
        uint8_t LENGTH = 0;
        uint8_t c = 0;


        for (c = 0; c < 19; c++)
        {
            /* Decode manchester byte by byte */
            payload[c] = manDecode(packet[c]);
        }

        /* Shift bits into LENGTH variable to find out which type of packet it is */
        LENGTH |= payload[0];
        LENGTH <<= 4;
        LENGTH |= payload[1];




        /* If the received RF packet is length 5*/
        if (LENGTH == 5)
        {
            /* Get payload into needed format to do CRC check */
            resultCrc = unpackFiveCRC(payload);

            /* Result of CRC */
            if (resultCrc != 1)
            {
                errorBlinkCase(4);

            }
            if(resultCrc == 1)
            {
                /* If CRC is ok, calculate data */
                       calculatedData = calculateFive();
            }
        }

        /* If the received RF packet is length 7*/
            if (LENGTH == 7)
            {
                resultCrc = unpackSevenCRC(payload);

                /* Result of CRC */
                if (resultCrc != 1)
                {
                    errorBlinkCase(4);
                }

                if(resultCrc == 1)
                {
                    calculatedData = calculateSeven();
                }
            }


            /* get time */
            t1 = time(NULL);
            /* put result in t struct */
            struct tm t = *localtime(&t1);



            /* TODO: make getters for filename so user can customize filename. For test purposes files are named LOG.txt
             * TODO: Make getter for timestamp so it can be removed from main
             * TODO: Make getter for ID in corresponding lib. For now it is called getIDTemp as it is a temporary function.
             */

            /* get ID, data type and unit*/
            idReturned = getId();

            String type = getType();
            String unit = getUnit();

            char bufferToSD[100];
            int cx;

            /*Open file*/
            fr = f_open(&fsrc, "LOG.txt", FA_OPEN_APPEND | FA_WRITE);

            /* Write JSON to buffer
             * TODO: PCB halts when trying to write double (%0.2f). Figure out solution
             * */

            cx = snprintf(bufferToSD, sizeof(bufferToSD),"\{\"Id\":%d\,\"Value\":%0.2f\,\"Unit\":\"%s\"\,\"Type\":\"%s\"\,\"TimeStamp\":\"%d-%02d-%02dT%02d:%02d:%02d\"\}\,",
                                                          idReturned,
                                                          calculatedData,
                                                          unit,
                                                          type,
                                                          (t.tm_year+1900),
                                                          (t.tm_mon+1),
                                                          t.tm_mday,
                                                          (t.tm_hour+2),
                                                          t.tm_min,
                                                          t.tm_sec);
           // cx = snprintf(bufferToSD, sizeof(bufferToSD),"\{\"Id\":%d\,\"Unit\":\"%s\"\,\"Type\":\"%s\"\,\"TimeStamp\":\"%d-%02d-%02dT%02d:%02d:%02d\"\}\,", idReturned,unit, type, year,month,day,hour,min,sec);

            /*Write buffer to file and close */
            fr = f_write(&fsrc, bufferToSD, cx, &bw);
            fr = f_close(&fsrc);
            errorBlinkCase(5);
    }

}

void callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if (e & RF_EventRxEntryDone)
    {

        /* Get current unhandled data entry */
        currentDataEntry = RFQueue_getDataEntry();

        /* Handle the packet data, located at &currentDataEntry->data:
         * - Length is fixed
         * - Data starts from the second byte */
        packetLength = 19;
        packetDataPointer = (uint8_t*) (&currentDataEntry->data + 1);

        /* Copy the payload + the status byte to the packet variable */
        memcpy(packet, packetDataPointer, (packetLength + 1));

        /* get next entry */
        RFQueue_nextEntry();

        /*Post semaphore to start packet processing */
        Semaphore_post(rxSemaphoreHandle);
    }
}
