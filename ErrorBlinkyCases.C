#include "ErrorBlinkyCases.H"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <xdc/std.h>
#include "Board.h"
#include <ti/drivers/PIN.h>
#include <ti/drivers/GPIO.h>

void errorBlinkCase(uint8_t blinkyCase)
{

    GPIO_write(Board_GPIO_RLED, 0);

    switch (blinkyCase)
    {

    case 1:
        /* SD Card error */
        /*long red led blink*/
        GPIO_toggle(Board_GPIO_RLED);
        CPUdelay(8000 * (1000));
        GPIO_toggle(Board_GPIO_RLED);
        CPUdelay(8000 * (1000));

        break;

    case 2:
        /* UART error */
        /*short red led blink*/
        GPIO_toggle(Board_GPIO_GLED);
        CPUdelay(2000 * (1000));
        GPIO_toggle(Board_GPIO_GLED);
        CPUdelay(2000 * (1000));
        break;

    case 3:
        /*RF error*/
        /*red and green alternate*/
        GPIO_toggle(Board_GPIO_RLED);
        CPUdelay(4000 * (1000));
        GPIO_toggle(Board_GPIO_RLED);
        CPUdelay(4000 * (1000));
        GPIO_toggle(Board_GPIO_GLED);
        CPUdelay(4000 * (1000));
        GPIO_toggle(Board_GPIO_GLED);
        CPUdelay(8000 * (1000));
        break;

    case 4:
        /* CRC nOk */
        GPIO_toggle(Board_GPIO_GLED);
        CPUdelay(2000 * (1000));
        GPIO_toggle(Board_GPIO_GLED);


        break;

    case 5:
        /* LOG SUCCESFULL */
        GPIO_toggle(Board_GPIO_RLED);
        CPUdelay(4000 * (1000));
        GPIO_toggle(Board_GPIO_RLED);
        break;

    }

}
