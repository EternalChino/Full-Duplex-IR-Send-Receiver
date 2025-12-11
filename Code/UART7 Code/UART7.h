//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#ifndef UART7_H_
#define UART7_H_

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void initUart7();
void setUart7BaudRate(uint32_t baudRate, uint32_t fcyc);
void putcUart7(char c);
void putsUart7(char* str);
char getcUart7();
bool kbhitUart7();

#endif
