/*
 * UartApi.c
 *
 *  Created on: 7 Jun 2022
 *      Author: Sjors Smit
 */

#include <UartApi.h>
#include <string.h>

#include <third_party/fatfs/ff.h>
#include <third_party/fatfs/diskio.h>
#include <third_party/fatfs/ffcio.h>
#include <third_party/fatfs/ffconf.h>

int uartApi_BaudRate = 115200; //Default to 115200
const int uartApi_stacksize = 1024;

#define UART_RX_SIZE 16 //Could be 10 since only listens for 9char commands
#define UART_TX_SIZE 128

//Size of message the UART bus listens for:
#define commandLength 9

char uartRxBuffer[UART_RX_SIZE + 1];
//char uartTxBuffer[UART_TX_SIZE + 1]; //Use local stack buffers instead of global tx buffer

char currentTask; //The current task that is running (read/write/erase etc) to know how to parse new messages
int currentIndex;   //Current "page" index when reading from SD file

// Test string to use as payload while still unable to load file from SD:
const char testPayload[] =
        "{\"data\":[{\"msg\":\"This is a fake string to send while the SD card is not yet working\"},{\"request\":\"Please implement a working version of SD card reading and replace it where this string is read, please and thank you\"}]}\n";

#define FILENAME "log.txt"
//Use separators in strings that do not interfere with potential JSON in payloads:
#define RESPONSE_TEMPLATE_I "cmd=%s/payload=%i\n" //Template when using int as payload
#define RESPONSE_TEMPLATE_S "cmd=%s/payload=%s\n" //Template when using string as payload

// Use these when allocating a char* for a payload response:
#define RESPONSE_TEMPLATE_SPACE_REQUIREMENT (sizeof(RESPONSE_TEMPLATE_S) + commandLength)
#define MAX_PAYLOAD_SIZE (UART_TX_SIZE - RESPONSE_TEMPLATE_SPACE_REQUIREMENT)

int uartApi_init()
{

    GPIO_init();
    UART_init();

    //Initialise the UART interface:
    UART_Params_init(&uartApi_params);
    uartApi_params.baudRate = uartApi_BaudRate;
    uartApi_params.writeDataMode = UART_DATA_BINARY;
    uartApi_params.readDataMode = UART_DATA_BINARY;
    uartApi_params.readReturnMode = UART_RETURN_NEWLINE;

    uartApi_params.readMode = UART_MODE_CALLBACK;
    uartApi_params.readCallback = (UART_Callback) uartApi_onReadCallback;

    uartApi_params.writeMode = UART_MODE_BLOCKING;
    uartApi_params.writeCallback = (UART_Callback) uartApi_onWriteCallback;

    uartApi_handle = UART_open(Board_UART0, &uartApi_params);
    if (uartApi_handle == NULL)
        return UARTAPI_OPENUART_FAILED;       //Return error

//    UART_read(uartApi_handle, uartRxBuffer, commandLength);
    UART_read(uartApi_handle, uartRxBuffer, commandLength);

    return 0;
}

/**
 * Send a test-string to the UART
 */
void uartApi_test()
{
    const char testString[] = "If you read this, Uart is working\r\n";
    UART_write(uartApi_handle, testString, sizeof(testString));
}

/**
 * Task in which the UART API runs
 */
void* uartApi_task(void *args)
{
    //Initialise a read call:
    UART_read(uartApi_handle, uartRxBuffer, commandLength);

    //Leave task alive: (is this even needed?)
    while (true)
    {
        sleep(1000);
    }
}

int listSdContents(char *buf, int len)
{
    DIR dir;        //Directory struct
    FILINFO fno;    //File info struct

//    SD_init();
//    SDFatFS_Handle sdCardHandle;
//    sdCardHandle = SDFatFS_open(Board_SD0, 0);
//    int_fast16_t sdResult = SD_initialize(sdCardHandle);

//    if (sdResult != SD_STATUS_SUCCESS)
//    {
//        char string[48];
//        snprintf(string, 47, "Open SD err: %i\n", sdResult);
//        strcat(FileSystemString, string);
//        return FileSystemString;
//    }

    FRESULT err = f_opendir(&dir, "/");
    if (err != FR_OK)
    {
        char string[48];
        snprintf(string, len - 1, "Open dir err: %i\n", err);
        strcat(buf, string);
//        SDFatFS_close(sdCardHandle);
        return err;
    }

    do
    {
        f_readdir(&dir, &fno);
        if (fno.fname[0] != 0)
        {
            if ((strlen(buf) + strlen(fno.fname)) < len - 2)
            {
                strcat(buf, fno.fname);
                strcat(buf, ",");  //Comma seperated
            }
            else
                return UARTAPI_BUFFER_FULL;
        }
    }
    while (fno.fname[0] != 0);

    buf[strlen(buf) - 1] = '\0';
    //Delete the last character in the string (trailing comma)
//    SDFatFS_close(sdCardHandle);
    return 0;
}

/**
 *
 */
UART_Callback uartApi_onReadCallback(UART_Handle handle, void *buf, size_t length)
{
    if (length <= commandLength)
    {
        char message[commandLength + 1]; //Message is max 9, plus a a null terminator

        //SNprintf received info:
        snprintf(message, commandLength + 1, "%s", (char*) buf);

        if (message[0] != '!')
        {
            char desync = commandLength;
            //if it is not at the start, find the location of it:
            char *ret = strchr(message, '!');
            if (ret != NULL)
            {
                desync = ret - message; //Check offset by comparing the pointers. is this okay to do?
                if (desync > commandLength)
                    desync = commandLength;
            }
            char response[commandLength + 1] = "!SYNC"; //Put in room for a null terminator that will not be sent with the message
            char offsetString[3];
            offsetString[0] = desync + '0'; //Turn single digit into string
            offsetString[1] = '\n';
            offsetString[2] = 0; //Add a null terminator

            strcat(response, offsetString);
            UART_write(uartApi_handle, response, strlen(response));
        }
        else
        {
            char CMD[9];
            //Commands are terminated with a '0' char (before the null terminator at the end of the string)
            unsigned char i;
            for (i = 0; i < 8; i++)
            {
                if (message[i] != '0')
                    CMD[i] = message[i];
                else
                {
                    //add a null terminator
                    CMD[i] = '\0';
                    break; //exit the loop
                }
            }
            //MAYBE: implement a "getCommandType" function and then use a switch case? Might look cleaner but probably won't be more efficient...
            if (!strcmp(CMD, "!INFO"))
            {
                char response[UART_TX_SIZE];
                {
                    char fileInfoPayload[MAX_PAYLOAD_SIZE];
                    //IMPLEMENTATION FOR SD CARD:
                    /*
                    uartApi_fileInfo fileinfo = uartApi_getFileInfo(FILENAME);
                    //sprintf file info into PAYLOAD
                    snprintf(fileInfoPayload, MAX_PAYLOAD_SIZE - 1, "{\"FileSize\": %i, \"packetcount\": %i}", fileinfo.fileLength,
                             fileinfo.packetCount);
                    */
                    //IMPLEMENTATION FOR TEST STRING:
                    uartApi_fileInfo fileinfo;
                    fileinfo.fileLength = strlen(testPayload);
                    fileinfo.packetCount = (strlen(testPayload) + MAX_PAYLOAD_SIZE -1)/MAX_PAYLOAD_SIZE;
                    snprintf(fileInfoPayload, MAX_PAYLOAD_SIZE - 1, "{\"FileSize\": %i, \"packetcount\": %i}", fileinfo.fileLength,
                             fileinfo.packetCount);
                    //sprintf payload into response:
                    snprintf(response, UART_TX_SIZE-1, RESPONSE_TEMPLATE_S, CMD, fileInfoPayload);
                }
                //Send the response
                uartApi_write(response);
            }
            else if (!strcmp(CMD, "!ERASE"))
            {
                char msg[UART_TX_SIZE];
                FRESULT ferr = f_unlink(FILENAME);
                snprintf(msg, UART_TX_SIZE - 1, RESPONSE_TEMPLATE_I, CMD, ferr);
                uartApi_write(msg);
            }
            else if (!strcmp(CMD, "!PING"))
            {
                uartApi_write("!PONG");
            }
            else if (!strcmp(CMD, "!LS"))
            {
                //uartApi_write("Listing\n");
                //sd contents returns calloc-ed string, remember to free:
                char sdString[MAX_PAYLOAD_SIZE] = ""; //Allocate with leaving room for rest of JSON encoding (see 3 lines below)
                int sdErr = listSdContents(sdString, sizeof(sdString));
                char msg[UART_TX_SIZE];
                snprintf(msg, UART_TX_SIZE - 1, RESPONSE_TEMPLATE_S, CMD, sdString);
                uartApi_write(msg);
            }
            else if (!strcmp(CMD, "!EPH"))
            {
                union TimeStamp
                {
                    char bytes[4];
                    uint32_t time;
                } timestamp;
                uint8_t i;
                for (i = 0; i < 4; i++)
                {
                    timestamp.bytes[i] = message[i + 5];
                }
                char response[UART_TX_SIZE];
                snprintf(response, UART_TX_SIZE - 1, RESPONSE_TEMPLATE_I, CMD, timestamp.time);
                uartApi_write(response);
            }
            else if (!strcmp(CMD, "!FILE"))
            {
                currentIndex = 0;
                char fileContents[MAX_PAYLOAD_SIZE];
                //Load from Sd:
//                uartApi_readFile(FILENAME, fileContents, MAX_PAYLOAD_SIZE-1, 0);
                //Load test string:
                uartApi_readTest(fileContents, MAX_PAYLOAD_SIZE - 1, 0);
                char response[UART_TX_SIZE];
                snprintf(response, UART_TX_SIZE-1, RESPONSE_TEMPLATE_S, CMD, fileContents);
                uartApi_write(response);

            }
            else if (!strcmp(CMD, "!NEXT"))
            {
                char fileContents[MAX_PAYLOAD_SIZE];
                //Load from Sd:
                //Check for out-of bounds:
//                uartApi_fileInfo fileinfo = uartApi_getFileInfo("log.txt");
                //
//                if(!(currentIndex >= fileInfo.fileLength))
//                {
//                uartApi_readFile(FILENAME, fileContents, MAX_PAYLOAD_SIZE - 1, currentIndex);
//                uartApi_write(fileContents);
//                }
                //Load test string:
                uartApi_readTest(fileContents, MAX_PAYLOAD_SIZE - 1, currentIndex);
                char response[UART_TX_SIZE];
                snprintf(response, UART_TX_SIZE-1, RESPONSE_TEMPLATE_S, CMD, fileContents);
                uartApi_write(response);
            }
            //Test command to create a new file
            else if (!strcmp(CMD, "!NEW"))
            {
                char fileContents[UART_TX_SIZE];
                int result = uartApi_writeFile(FILENAME, fileContents, UART_TX_SIZE - 1, 0);
                char msg[UART_TX_SIZE];
                snprintf(msg, UART_TX_SIZE - 1, RESPONSE_TEMPLATE_I, CMD, result);
                uartApi_write(msg);
            }
            else
            {
                UART_write(uartApi_handle, "Unknown-command\n", sizeof("Unknown command\n"));
            }

        }

    }

    //Re-open the UART read:
    UART_read(uartApi_handle, uartRxBuffer, commandLength);
    return 0;
}

int uartApi_write(const char *msg)
{
    if (strlen(msg) > UART_TX_SIZE)
        return -1;
    UART_write(uartApi_handle, msg, strlen(msg));
    return 0;
}

UART_Callback uartApi_onWriteCallback(UART_Handle handle, void *buf, size_t length)
{
    return 0;
}

void uartApi_close()
{
    UART_close(uartApi_handle);
}

uartApi_fileInfo uartApi_getFileInfo(char *fileName)
{
    //Open Sd card:
//    SD_init();
//    SDFatFS_Handle sdCardHandle;
//    sdCardHandle = SDFatFS_open(Board_SD0, 0);

    uartApi_fileInfo fileSizeInfo;
    fileSizeInfo.fileLength = 0;
    fileSizeInfo.packetCount = 0;

    FIL file;

    FRESULT ferr = f_open(&file, fileName, FA_READ);
    if (ferr == FR_OK)
    {
        FSIZE_t fileSize = f_size(&file);
        fileSizeInfo.fileLength = fileSize; //copy over filesize
        //Round up filesize/packetsize
        fileSizeInfo.packetCount = (fileSize + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
    }
//    SDFatFS_close(sdCardHandle);
    f_close(&file);
    return fileSizeInfo;

}

int uartApi_readFile(const char *filename, char *buf, UINT len, UINT offset)
{
    SD_init();
    SDFatFS_Handle sdCardHandle;
    sdCardHandle = SDFatFS_open(Board_SD0, 0);

    FIL file;
//    BYTE readBuffer[UART_TX_SIZE];

    UINT readCount;
    UINT amountToRead = len;
    FRESULT ferr = f_open(&file, filename, FA_READ | FA_OPEN_ALWAYS);
    if (ferr == FR_OK)
    {
        ferr = f_read(&file, buf, amountToRead, &readCount);
        if (ferr != FR_OK)
        {
            snprintf(buf, amountToRead, "Couldn't read file %i - %i\n", ferr, readCount);
        }
    }
    else
    {
        snprintf(buf, amountToRead, "Couldn't open file %i\n", ferr);
    }
    f_close(&file);
//    SDFatFS_close(sdCardHandle);
    return ferr;
}

int uartApi_writeFile(const char *filename, char *buf, UINT len, UINT offset)
{
    SD_init();
    SDFatFS_Handle sdCardHandle;
    sdCardHandle = SDFatFS_open(Board_SD0, 0);

    FIL file;

    UINT readCount;
    UINT amountToRead = len;
    FRESULT ferr = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (ferr == FR_OK)
    {
//        ferr = f_write(&file, buf, strlen(readBuffer), &readCount);
//        if (ferr != FR_OK)
//        {
//            snprintf(buf, amountToRead, "Couldn't read file %i - %i\n", ferr, readCount);
//        }
    }
    else
    {
        snprintf(buf, amountToRead, "Couldn't open file %i\n", ferr);
    }
    f_close(&file);
//    SDFatFS_close(sdCardHandle);
    return ferr;
}

int uartApi_readTest(char *buf, uint16_t len, uint16_t offset)
{
    memset(buf, 0, len);
    strncpy(buf, &testPayload[offset], len - 1);
    //Advance the index counter for subsequent reads
    currentIndex += strlen(buf);

    //If read amount of bytes is shorter than bytes requested, end of file is reached
    if (strlen(buf) < (len - 1))
        currentIndex = 0;

    return 0;
}

/*******************************************************
 *
 * DEPRECATED
 *
 ****************************************************/

// For thread (no longer used)
pthread_t uartApi_thread;
pthread_attr_t uartApi_attrs;
struct sched_param uartApi_priParam;
int uartApi_retc;
int uartApi_detachState;

/**
 * Load the UART API task on the pthread scheduler
 */
int uartApi_runTask()
{
    //initialise the thread:
    pthread_attr_init(&uartApi_attrs);
    uartApi_priParam.sched_priority = 5;
    uartApi_detachState = PTHREAD_CREATE_DETACHED;
    uartApi_retc = pthread_attr_setdetachstate(&uartApi_attrs, uartApi_detachState);

    uartApi_retc |= pthread_attr_setstacksize(&uartApi_attrs, uartApi_stacksize);

    if (uartApi_retc != 0)
        return UARTAPI_SETSTACKSIZE_FAILED; //Thread setStacksize failed

    uartApi_retc = pthread_create(&uartApi_thread, &uartApi_attrs, uartApi_task,
    NULL);
    if (uartApi_retc != 0)
        return UARTAPI_THREAD_FAILED;

    return 0;
}
void uartApi_stopTask()
{

}

