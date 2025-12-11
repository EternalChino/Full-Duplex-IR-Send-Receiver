#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

#define UART_TX_MASK 2  // PE1 mask for TX (binary 0b10)
#define UART_RX_MASK 1  // PE0 mask for RX (binary 0b01)

// -------------------- Initialize UART7 --------------------
void initUart7()
{
    // Enable clocks for UART7 and Port E
    SYSCTL_RCGCUART_R |= SYSCTL_RCGCUART_R7;  // Enable UART7 clock
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R4;  // Enable Port E clock
    _delay_cycles(3);  // Small delay to allow clock to stabilize

    // Configure UART7 pins (PE0 = RX, PE1 = TX)
    GPIO_PORTE_DEN_R |= UART_TX_MASK | UART_RX_MASK;    // Enable digital functionality on PE0, PE1
    GPIO_PORTE_AFSEL_R |= UART_TX_MASK | UART_RX_MASK;  // Enable alternate function for UART
    GPIO_PORTE_PCTL_R &= ~(GPIO_PCTL_PE1_M | GPIO_PCTL_PE0_M); // Clear previous pin configuration
    GPIO_PORTE_PCTL_R |= GPIO_PCTL_PE1_U7TX | GPIO_PCTL_PE0_U7RX;  // Assign UART7 TX and RX to PE1, PE0

    // Configure UART7 for 115200 baud assuming 40 MHz system clock
    UART7_CTL_R = 0;                  // Disable UART7 before configuring
    UART7_CC_R = UART_CC_CS_SYSCLK;   // Select system clock (40 MHz)
    UART7_IBRD_R = 21;                // Integer part of baud rate divisor (40 MHz / (16 * 115200))
    UART7_FBRD_R = 45;                // Fractional part of baud rate divisor
    UART7_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN | UART_LCRH_PEN | UART_LCRH_EPS; // 8-bit data, FIFO enabled, parity enabled, even parity
    UART7_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;  // Enable UART7, TX, RX
}

// -------------------- Set UART7 Baud Rate --------------------
void setUart7BaudRate(uint32_t baudRate, uint32_t fcyc)
{
    // Calculate divisor times 128 for improved precision
    uint32_t divisorTimes128 = (fcyc * 8) / baudRate;
    divisorTimes128 += 1;  // Round to nearest 128th

    UART7_CTL_R = 0;  // Disable UART7 to allow safe configuration
    UART7_IBRD_R = divisorTimes128 >> 7;           // Integer part of divisor
    UART7_FBRD_R = ((divisorTimes128) >> 1) & 63; // Fractional part (6 bits)
    UART7_LCRH_R = UART_LCRH_WLEN_8 | UART_LCRH_FEN | UART_LCRH_PEN | UART_LCRH_EPS;
    // 8-bit data, FIFO enabled, parity enabled, even parity
    UART7_CTL_R = UART_CTL_TXE | UART_CTL_RXE | UART_CTL_UARTEN;  // Re-enable UART7, TX, RX
}

// -------------------- Write a single character --------------------
void putcUart7(char c)
{
    while (UART7_FR_R & UART_FR_TXFF);  // Wait while TX FIFO is full
    UART7_DR_R = c;                     // Write character to data register
}

// -------------------- Write a string --------------------
void putsUart7(char* str)
{
    uint8_t i = 0;
    while (str[i] != '\0')
        putcUart7(str[i++]);  // Send each character one by one
}

// -------------------- Read a single character --------------------
char getcUart7()
{
    while (UART7_FR_R & UART_FR_RXFE);  // Wait while RX FIFO is empty
    return UART7_DR_R & 0xFF;           // Return received character (lower 8 bits)
}

// -------------------- Check if UART7 has received data --------------------
bool kbhitUart7()
{
    return !(UART7_FR_R & UART_FR_RXFE);  // True if RX FIFO is not empty
}
