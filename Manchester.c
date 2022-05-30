#include <stdlib.h>
#include<stdio.h>
#include<string.h>
#include "Manchester.h"

uint8_t test[4];


uint8_t manDecode(uint8_t d)
 {
    int count = 0; // count used for saving in the right place in the array
    int i;
    for(i = 0; i<8; i+=2) // plus 2 because manchester encodes 1 bit as 2 bits. So 01 = 0 and 10 = 1
    {
       uint8_t temp = d; // temp is the temporary value we are shifting E.G: 01011010 which is equal to dec 90.

       temp = temp << i; // We want to check every two bites. So first we shift 0 left and 6 right so we are left with 00000001. Next time we remove the first two bits and shift back again so we are left with 00000001 etc.
       temp = temp >> 6;

            if(temp == 1) // if the shifted value = 00000001 it is 1 in binary which is 0 in manchester and vice versa.
                {
                    test[count] = 0;

                }
            else
                {
                    test[count] = 1;
                }

        count++; // add 1 to count
    }


    uint8_t decoded = 0;

    for(i = 0; i<4; i++) // from array to uint
        {
            decoded = decoded | test[i];
            decoded = decoded << 1;
        }
decoded = decoded >> 1;
    return(decoded); // after it's done return the test array.

 }
