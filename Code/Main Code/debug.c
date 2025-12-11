
#include "debug.h"

char* toAsciiHex(char *buff, uint32_t Val)
{
    int8_t i;
    for (i = 7; i >= 0; i--)
    {
        uint8_t hex = Val & 0x0F;

        if (hex < 10)
            buff[i] = '0' + hex;
        else
            buff[i] = 'A' + (hex - 10);

        Val = Val >> 4;
    }
    buff[8] = '\0';
return buff;
}
