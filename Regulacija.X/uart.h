/* 
 * File:   uart.h
 * Author: Milankov
 *
 * Header fajl za rad sa uart-om na pic16f887
 * Created on 11.04.2016., 14.27
 */
#include<xc.h>

#define _XTAL_FREQ 8000000

#define TX_PIN RC6
#define TX_PIN_DIR TRISCbits.TRISC6
#define RX_PIN RC7
#define RX_PIN_DIR TRISCbits.TRISC7

char UARTInit(const unsigned long);  //uart init
void UARTWrite(char);
char UARTTxFull();  //returns TMRT value
void UARTWriteString(char *);
