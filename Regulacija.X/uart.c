#include "uart.h"

//datasheet 16f887 strane od 157.
char UARTInit(const unsigned long baud_rate)
{
    unsigned int x;
    unsigned char k;

    TX_PIN_DIR = 0;
    RX_PIN_DIR = 0;
    INTCON |= 0b11000000;   //global and pripherial enable interrupt
    PIE1bits.RCIE = 1;  //Rx interrupt enable

    if((baud_rate > 115200) || (baud_rate < 300)) return 0;
    else
    {
        k = 4;  //coeficient for baud rate formula
    }

    if(baud_rate == 9600) x = (_XTAL_FREQ / baud_rate)/k - 1;   //formula for 9600
    else x = (_XTAL_FREQ / baud_rate)/k ;   //formula for other baud rates

    BRGH = 1;
    BRG16 = 1;
    SPBRG = x & 0xFF;                             //Writing SPBRG Register
    SPBRGH = (x>>8) & 0xFF;                       //Writing SPBRGH register
    SYNC = 0;                                     //Setting Asynchronous Mode,
    SPEN = 1;                                     //Enables Serial Port
    CREN = 1;                                     //Enables Continuous Reception
    TXEN = 1;                                     //Enables Transmission

    return 1;
}

void UARTWrite(char data)
{
  while(!TRMT);     //wait for transmit buffer to empty
  TXREG = data;
}

char UARTTxFull()
{
  return TRMT;
}

void UARTWriteString(char *str)
{
  int i;
  for(i=0;str[i]!='\0';i++)
    UARTWrite(str[i]);
}