/*
 * UartApi.h
 *
 *  Created on: Jun 2022
 *      Author: Sjors Smit
 */

#ifndef UARTAPI_H
#define UARTAPI_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <ti/sysbios/BIOS.h>

#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/SD.h>
#include <ti/drivers/uart/UARTCC26XX.h>
#include <string.h>
#include <stdio.h>
#include "Board.h"


#include "ti/drivers/SDFatFS.h"
/************************************************
 *  IMPORTANT NOTE
 *  This library is not written thread/resource safe and might give unexpected results if the UART0 interface is used in other tasks
 */


//For UART:
UART_Handle uartApi_handle;
UART_Params uartApi_params;

typedef struct uartApi_fileInfo{
    int fileLength;
    int packetCount;
}uartApi_fileInfo;

int uartApi_init();

void uartApi_test();
int listSdContents(char* buf, int len);

void* uartApi_task(void* args);

int uartApi_runTask();
void uartApi_stopTask();

int uartApi_readFile(const char* filename, char* buffer, UINT len, UINT offset);
int uartApi_readTest(char* buf, uint16_t len, uint16_t offset);
int uartApi_writeFile(const char* filename, char* buffer, UINT len, UINT offset);

void uartApi_close();
UART_Callback uartApi_onReadCallback(UART_Handle handle, void* buf, size_t length);
UART_Callback uartApi_onWriteCallback(UART_Handle handle, void* buf, size_t length);
uartApi_fileInfo uartApi_getFileInfo(char* fileName);

int uartApi_write(const char* msg);


//ERROR CODES:
#define UARTAPI_OPENUART_FAILED -101
#define UARTAPI_SETSTACKSIZE_FAILED -201
#define UARTAPI_THREAD_FAILED -202
#define UARTAPI_BUFFER_FULL -301

#endif /* UARTAPI_H_ */
