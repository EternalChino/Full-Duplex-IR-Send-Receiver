#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "clock.h"
#include "uart0.h"
#include "debug.h"
#include "uart7.h"

#define PB6 0x40                 // Mask for PWM output pin PB6
#define MAX_CHARS 80             // Maximum number of characters in user input
#define MAX_FIELDS 5             // Maximum number of fields parsed from input

// Structure to store user input and parsed fields
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];       // Input string buffer
    uint8_t fieldCount;             // Number of fields in the buffer
    uint8_t fieldPosition[MAX_FIELDS]; // Start index of each field
    char fieldType[MAX_FIELDS];     // Type of each field: 'a' = alpha, 'n' = numeric
} USER_DATA;

// -------------------- UART0 Input --------------------
void getsUart0(USER_DATA *user_data)
{
    uint32_t count = 0;      // Counter for characters
    char bufferc;             // Temporary storage for each received character

    while (1)
    {
        bufferc = getcUart0(); // Read a character from UART0

        if ((bufferc == 8 || bufferc == 127) && count > 0 ) // Handle backspace
        {
            count--;
        }
        else if (bufferc == 13) // Enter key detected
        {
            user_data->buffer[count] = '\0'; // Null terminate string
            break;
        }
        else if (bufferc >= 32 && count < MAX_CHARS) // Valid ASCII characters
        {
            user_data->buffer[count++] = bufferc;  // Store in buffer
        }
        else if (count == MAX_CHARS) // Maximum length reached
        {
            user_data->buffer[count] = '\0'; // Null terminate
        }
    }
}

// -------------------- PWM Initialization --------------------
void initPwm(void)
{
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0; // Enable PWM module 0
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R1; // Enable Port B
    _delay_cycles(3);

    GPIO_PORTB_AFSEL_R |= PB6;                  // Enable alternate function on PB6
    GPIO_PORTB_PCTL_R &= ~GPIO_PCTL_PB6_M;     // Clear PCTL bits
    GPIO_PORTB_PCTL_R |= GPIO_PCTL_PB6_M0PWM0; // Set PB6 to M0PWM0
    GPIO_PORTB_DEN_R |= PB6;                    // Enable digital function
    GPIO_PORTB_AMSEL_R &= ~PB6;                // Disable analog function

    SYSCTL_RCC_R |= SYSCTL_RCC_USEPWMDIV | SYSCTL_RCC_PWMDIV_4; // PWM clock = sysclk / 4

    PWM0_0_CTL_R = 0;                           // Disable PWM0 generator 0
    PWM0_0_GENA_R = PWM_0_GENA_ACTCMPAD_ZERO | PWM_0_GENA_ACTLOAD_M; // Action on A
    PWM0_0_GENB_R = PWM_0_GENB_ACTCMPBD_ZERO | PWM_0_GENB_ACTLOAD_M; // Action on B

    PWM0_0_LOAD_R = 263;   // Set period (LOAD value)
    PWM0_0_CMPA_R = 142;   // Set duty cycle (CMPA value)

    PWM0_ENABLE_R |= PWM_ENABLE_PWM0EN; // Enable PWM output
    PWM0_0_CTL_R |= PWM_0_CTL_ENABLE;   // Enable PWM generator
}

// -------------------- LED Initialization --------------------
void initRedLed(void)
{
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5; // Enable Port F
    _delay_cycles(3);
    GPIO_PORTF_DIR_R |= 0x06;  // PF1 (red) and PF2 (blue) as output
    GPIO_PORTF_DEN_R |= 0x06;  // Digital enable PF1 and PF2
    GPIO_PORTF_DATA_R &= ~0x06; // Turn LEDs off initially
}

// -------------------- UART7 Interrupt Handler --------------------
void UART7_Handler(void)
{
    uint32_t mis = UART7_MIS_R; // Read masked interrupt status

    // Check if RX interrupt occurred
    if(mis & UART_MIS_RXMIS)
    {
        uint32_t errors = UART7_RSR_R;  // Read error flags

        if(errors & (UART_RSR_PE | UART_RSR_FE | UART_RSR_OE)) // Check for parity, framing, or overrun errors
        {
            putsUart0("\n[UART7 ERROR] "); // Report errors to terminal

            if(errors & UART_RSR_PE)
                putsUart0("Parity ");
            if(errors & UART_RSR_FE)
                putsUart0("Framing ");
            if(errors & UART_RSR_OE)
                putsUart0("Overrun ");

            putsUart0("Error\n");

            volatile char discard = UART7_DR_R; // Read to clear error
            UART7_ICR_R = UART_ICR_RXIC;        // Clear RX interrupt
        }
        else
        {
            char c = UART7_DR_R & 0xFF;        // Read normal data
            UART7_ICR_R = UART_ICR_RXIC;       // Clear RX interrupt
        }
    }
}

// -------------------- Command Parsing --------------------
void parseFields(USER_DATA *user_data)
{
    uint32_t count = 0;
    uint32_t fieldCount = 0;
    uint32_t firstDelimiter = 1;
    char alpha;
    char numeric;
    char delimiter;

    while (1)
    {
        char bufferc = user_data->buffer[count];
        if (bufferc == '\0') break;

        // Determine character type
        alpha = (( bufferc >= 'A' && bufferc <= 'Z') || (bufferc >= 'a' && bufferc <= 'z'));
        numeric = ((bufferc <= '9' && bufferc >= '0') || bufferc == '-' || bufferc == '.');
        delimiter = !(alpha || numeric);

        // New field detected
        if (firstDelimiter && !delimiter)
        {
            if (fieldCount < MAX_FIELDS)
            {
                user_data->fieldPosition[fieldCount] = count;
                user_data->fieldType[fieldCount] = alpha ? 'a' : 'n';
                fieldCount++;
            }
            else
            {
                break;
            }
        }

        if (delimiter) // End of field
        {
            user_data->buffer[count] = '\0'; // Null terminate
            firstDelimiter = 1;
        }
        else
        {
            firstDelimiter = 0;
        }
        count++;
    }
    user_data->fieldCount = fieldCount; // Store number of fields
}

char *getFieldString(USER_DATA *user_data, uint8_t fieldNumber)
{
    if (user_data == 0) return 0;
    if (fieldNumber >= user_data->fieldCount) return 0;

    return &(user_data->buffer[user_data->fieldPosition[fieldNumber]]); // Return pointer to field
}

int32_t getFieldInteger(USER_DATA *user_data, uint8_t fieldNumber)
{
    if (user_data == 0) return 0;
    if (fieldNumber >= user_data->fieldCount) return 0;

    char *string = &(user_data->buffer[user_data->fieldPosition[fieldNumber]]);
    uint32_t num = 0;
    bool negative = false;
    uint8_t i = 0;

    if (string[0] == '-') { negative = true; i++; } // Negative number

    while (string[i] >= '0' && string[i] <= '9') // Convert digits
    {
        num = num * 10 + (string[i] - '0');
        i++;
    }
    if (negative) num = -num;

    return num;
}

bool isCommand(USER_DATA *user_data, const char strCommand[], uint8_t minArguments)
{
    if (user_data == 0 || user_data->fieldCount == 0) return false;

    char *command = &(user_data->buffer[user_data->fieldPosition[0]]);
    uint32_t i = 0;

    // Compare command strings
    while (command[i] != '\0' && strCommand[i] != '\0')
    {
        if (command[i] != strCommand[i]) return false;
        i++;
    }

    if (command[i] != '\0' || strCommand[i] != '\0') return false;
    if ((user_data->fieldCount - 1) < minArguments) return false;

    return true;
}

bool strCompare(const char *string1, const char *string2)
{
    uint32_t i = 0;
    while (string1[i] != '\0' && string2[i] != '\0')
    {
        if (string1[i] != string2[i]) return false;
        i++;
    }
    return (string1[i] == '\0' && string2[i] == '\0'); // True if strings equal
}

void intToStr(uint32_t num, char *buffer)
{
    char temp[11];
    int i = 0, j = 0;

    if (num == 0)
    {
        buffer[0] = '0';
        buffer[1] = '\0';
        return;
    }

    while (num > 0)
    {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }

    while (i > 0)
    {
        buffer[j++] = temp[--i]; // Reverse string
    }
    buffer[j] = '\0';
}

// -------------------- Main Program --------------------
int main()
{
    initSystemClockTo40Mhz(); // Set system clock to 40 MHz
    initUart0();              // Terminal UART
    initUart7();              // PE0/PE1 UART
    initPwm();                // PWM output
    initRedLed();             // LEDs for status
    setUart7BaudRate(300, 40000000); // UART7 baud rate

    USER_DATA user_data;
    putsUart0("Command interface ready.\n");

    while (1)
    {
        // -------- RECEIVE ON PE0 (UART7_RX) --------
        if (kbhitUart7())
        {
            GPIO_PORTF_DATA_R ^= 0x04;  // Toggle blue LED PF2
            char incoming = getcUart7();
            putcUart0(incoming);        // Echo to terminal
            GPIO_PORTF_DATA_R &= ~0x04; // Turn off blue LED
        }

        // -------- READ LINE FROM TERMINAL (UART0) --------
        if (kbhitUart0())
        {
            getsUart0(&user_data);       // Blocking read until Enter
            parseFields(&user_data);     // Split input into fields

            if (isCommand(&user_data, "send", 1)) // Command "send"
            {
                uint8_t i;
                for (i = 1; i < user_data.fieldCount; i++)
                {
                    char *str = &(user_data.buffer[user_data.fieldPosition[i]]);
                    uint32_t j = 0;
                    while (str[j] != '\0')
                    {
                        putcUart7(str[j++]); // Send each character over UART7
                    }
                    if (i < user_data.fieldCount - 1)
                        putcUart7(' ');    // Add space between fields
                }
                putcUart7(0);            // Null terminator
                putsUart0("Sent.\n");    // Confirmation on terminal
            }
            else
            {
                putsUart0("Unknown command.\n"); // Invalid command
            }
        }
    }

    return 0;
}
