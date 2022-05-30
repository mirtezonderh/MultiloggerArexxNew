#include <stdlib.h>
#include<stdio.h>
#include <stdint.h>
#include <EpochSet.h>
#include <ti/drivers/UART.h>
#include <ti/drivers/GPIO.h>
#include "Board.h"

/* TODO: Uart get initialized in this function. For now this is OK but this has to be moved so the UART can be
 *       accesed in other parts of the code.
 */

uint32_t getEpoch(){

    char input;
    uint8_t epocharray[4];
    uint32_t epoch = 0;

     GPIO_init();
     UART_init();

     UART_Handle uart;
     UART_Params uartParams;

     /* Create a UART with data processing off. */
     UART_Params_init(&uartParams);
     uartParams.writeDataMode = UART_DATA_BINARY;
     uartParams.readDataMode = UART_DATA_BINARY;
     uartParams.readReturnMode = UART_RETURN_FULL;
     uartParams.readEcho = UART_ECHO_OFF;
     uartParams.baudRate = 115200;

     uart = UART_open(Board_UART0, &uartParams);

     if (uart == NULL)
     {
         /* UART_open() failed */
         while (1)
             ;
     }
     int ix = 0;
          for (ix = 0; ix < 4; ix++)
          {
              /* input from python script is epoch in 4 bytes. Read and process byte by byte*/
              UART_read(uart, &input, 1);
              UART_write(uart, &input, 1);
              UART_write(uart, "\n", 1);
              epocharray[ix] = 0;
              epocharray[ix] ^= input;
              /* Bytes are in big indian, shift every byte in the uint32_t. */
              epoch ^= epocharray[ix];

              if (ix < 3)
              {
                  /* don't shift the 4th byte*/
                  epoch <<= 8;
              }
          }
          UART_close(uart);

          return epoch;


}


