/*
 * File:   main.c
 * Author: Milankov
 *
 * Created on 31.07.2015., 18.00
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "sht71.h"
#include "ds18b20.h"
#include "tgs4161.h"
#include "uart.h"

//tasteri
#define MENU RA4
#define MINUS RA3
#define PLUS RA2
#define OK RA1

//izlazi
#define GREJAC RC4
#define ISPARIVAC RC1
#define PWM RC2
#define VENTILATOR RC0
#define KLIMA RC3
#define TGS_OFF RA5

#define HISTEREZIS_TEMP 0.4
#define HISTEREZIS_HUMI 2.5
#define HISTEREZIS_CO2 250
#define PODEOK_TEMP 0.1
#define PODEOK_HUMI 1
#define PODEOK_CO2 100


// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
//#pragma config WDTE = ON        // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Reset Selection bits (BOR enabled)
#pragma config IESO = ON        // Internal External Switchover bit (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

char ok_flag, ok_flag_humi, ok_flag_co2, menu_flag, plus_flag, minus_flag;
unsigned char measure , disp_count, measure_count, disp, measure_co2;
unsigned char humidity[5]={0,0,0,0,0};
unsigned char temperature[5]={0,0,0,0,0};
//char uart_data[30], str[5]; //za uart
float temp, humi, t;
__eeprom float zeljena_temperatura;
__eeprom float zeljena_vlaznost;
__eeprom int zeljena_co2;
unsigned int co2, tmr_co2;
unsigned int tmr_count;

static void interrupt isr(){
    if(INTCONbits.T0IF)
    {
        INTCONbits.T0IF = 0;    //interrupt flag clear
        TMR0 = 0;
        return;
    }

    //INTCONbits.T0IF = 0;    //interrupt flag clear
    PIR1bits.TMR1IF = 0;

//    if(tmr_co2 == 18000) //priblizno 3 min (5450 = 180/0.033)
//    {
//        tmr_co2 = 0;
//        TGS_OFF = ~TGS_OFF;
//    }
//    if((!TGS_OFF) && (tmr_co2 > 12000))  //senzor je ukljucen duze od 2 minuta
//    {
//        measure_co2 = 1;    //mogu se vrsiti merenja
//    }
//    else measure_co2 = 0;   // ne vrse se merenja

    if(tmr_count == 300)        // 3 sekunde
    {
        tmr_count = 0;
        measure = 1;
    }
    else if(tmr_count == 200)  // 2 sekunde
    {
        disp = 1;
    }
    tmr_count++;    //obican brojac prekida
    //tmr_co2++;      //brojac prekida koji ce sluziti za odredj. vremena uklj. i isklj. senzora co2
    //TMR0 = 0;
    TMR1H = 0xD8;
    TMR1L = 0xEF;
}

void initWDT()
{
    WDTCONbits.SWDTEN = 1;
    WDTCONbits.WDTPS = 0b0110;
}

void IOPinsConfig(){
    TRISA |= 0b00011110;    //pinovi na kojima su tasteri su ulazni
    TRISA &= 0b11011111;    //RA5  ukljucuje napajanje TGS logickom 1
    ANSEL &= 0b00000001;    //pinovi na kojima su tasteri su digitalni
    TRISC &= 0b10000000;    //izlazni pinovi
    TRISC |= 0b10000000;
}

void IOPinsInit(){
    GREJAC = 0;
    ISPARIVAC = 0;
    VENTILATOR = 0;
    KLIMA = 0;
    TGS_OFF = 0;
}

void displayAirTemp(float temp2){
    sprintf(temperature, " %.1f", temp2);
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Air temperature ");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString((char *)" deg        ");
}

void displayAirMoist(float humi2){
    sprintf(humidity, " %.1f", humi2);
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Air moist       ");
    LcdSetCursor(2,1);
    LcdWriteString(humidity);
    LcdSetCursor(2,6);
    LcdWriteString((char *)" %          ");
}

void displayCO2(unsigned int co22){
    LcdSetCursor(1,1);
    LcdWriteString((char *)"CO2 in air      ");
    if(measure_co2)
    {
        LcdWriteInt(co22,2,1);
        LcdSetCursor(2,5);
        LcdWriteString((char *)" ppm        ");
    }
    else
    {
        LcdSetCursor(2,1);
        LcdWriteString((char *)"Measuring...    ");
    }
}

void displaySoilTemp(float t2){
    sprintf(temperature, " %.1f", t2);
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Soil temperature");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString((char *)" deg        ");
}

void resetValues(){ //f-ja koja dodeljuje pocetne vrednosti promenljivima
    ok_flag = 0;
    menu_flag = 0;
    plus_flag = 0;
    minus_flag = 0;
    tmr_count = 0;
    measure = 0;
    disp_count = 0;
    measure_count = 0;
    disp = 1;
    //zeljena_temperatura = 24;
    //zeljena_vlaznost = 90;
    //zeljena_co2 = 1000;
    ok_flag_humi = 0;
    ok_flag_co2 = 0;
    tmr_co2 = 0;
    measure_co2 = 1;
    co2 = 0;
}

void initTimer0(){  //f-ja koja konfigurise timer0, prekidi svakih 32,76ms
    OPTION_REGbits.T0CS = 0;    //timer 0 clock source, 0 znaci Fosc/4
    OPTION_REGbits.PSA = 1; //prescaler dodeljen watchdog
    OPTION_REG |= 0b00000111;   //prescaler 1:256
    INTCONbits.GIE = 1; //globalni prekidi dozvoljeni
    TMR0 = 0;   //256*256*0,5us = prekid svakih 32,76ms
    INTCONbits.T0IE = 1;    //prekid timer0 dozvoljen
}

void initTimer1()
{
    T1CONbits.T1CKPS = 0b01;        // 1:2 prescaler
    TMR1H = 0xD8;
    TMR1L = 0xEF;                   // Interrupt every 10 ms
    PIE1bits.TMR1IE = 1;            // Enable TMR1 interrupts
    INTCONbits.GIE = 1;             // Enable global interrupts
    T1CONbits.TMR1ON = 1;           // Timer on
}

void regulacija(){
    //temperatura
    if(GREJAC)  //grejanje
    {
        if(temp > zeljena_temperatura)
        {
            GREJAC = 0;
        }
    }
    else if(temp < (zeljena_temperatura - HISTEREZIS_TEMP))
    {
        GREJAC = 1;
    }
    
    if(KLIMA)   //hladjenje
    {
        if(temp < zeljena_temperatura)
        {
            KLIMA = 0;
        }
    }
    else if(temp > (zeljena_temperatura + HISTEREZIS_TEMP))
    {
            KLIMA = 1;
    }
    

    
    //vlaznost
    if(ISPARIVAC)
    {
        if(humi > (zeljena_vlaznost + HISTEREZIS_HUMI))
        {
            ISPARIVAC = 0;
        }
    }
    
    else if(humi < (zeljena_vlaznost - HISTEREZIS_HUMI))
    {
        ISPARIVAC = 1;
    }


    //CO2
    if(measure_co2) //ako se vrse merenja
    {
         if (VENTILATOR)
        {
            if(co2 < (zeljena_co2 - HISTEREZIS_CO2))
            {
                VENTILATOR = 0;
            }
        }

        else if(co2 > (zeljena_co2 + HISTEREZIS_CO2))
        {
            VENTILATOR = 1;
        }
    }
}

void manage_buttons()
{
    if(PLUS)
    {
        __delay_ms(20);
        if(PLUS) plus_flag = 1;
        __delay_ms(150);
    }

    if(MINUS)
    {
        __delay_ms(20);
        if(MINUS) minus_flag = 1;
        __delay_ms(150);
    }

    if(OK)    //debouncing
    {
        __delay_ms(20);
        if(OK)  ok_flag = 1;
        while(OK);
        __delay_ms(20);
    }
}

void menuCO2(){
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Set CO2 in air  ");
    LcdSetCursor(2,1);
    LcdWriteString((char *)"          ");
    LcdSetCursor(2,1);
    LcdWriteInt(zeljena_co2,2,1);
    LcdSetCursor(2,6);
    LcdWriteString((char *)"        ");
    while(!ok_flag)
    {
        CLRWDT();                           // Reset watchdog timer
        
        manage_buttons();

        if(plus_flag)
        {
            zeljena_co2 += PODEOK_CO2;
            if(zeljena_co2 > 5000) zeljena_co2 = 5000;  //zastita
            plus_flag = 0;

            LcdSetCursor(2,1);
            LcdWriteInt(zeljena_co2,2,1);
        }

        if(minus_flag)
        {
            zeljena_co2 -= PODEOK_CO2;
            if(zeljena_co2 < 300) zeljena_co2 = 300;    //zastita
            minus_flag = 0;

            LcdSetCursor(2,1);
            LcdWriteInt(zeljena_co2,2,1);

        }

        if(ok_flag)
        {
            ok_flag = 1;
            ok_flag_co2 = 1;
        }
    }
}

void menuHumi(){
    sprintf(humidity, " %.1f", zeljena_vlaznost);
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Set air moist   ");
    LcdSetCursor(2,1);
    LcdWriteString(humidity);
    LcdSetCursor(2,6);
    LcdWriteString((char *)"        ");

    while(!ok_flag_co2)
    {
        CLRWDT();                           // Reset watchdog timer

        manage_buttons();

        if(plus_flag)
        {
            zeljena_vlaznost += PODEOK_HUMI;
            if(zeljena_vlaznost > 99) zeljena_vlaznost = 99;
            plus_flag = 0;

            sprintf(humidity, " %.1f", zeljena_vlaznost);
            LcdSetCursor(2,1);
            LcdWriteString(humidity);
        }

        if(minus_flag)
        {
            zeljena_vlaznost -= PODEOK_HUMI;
            if(zeljena_vlaznost < 30) zeljena_vlaznost = 30;    //zastita
            minus_flag = 0;

            sprintf(humidity, " %.1f", zeljena_vlaznost);
            LcdSetCursor(2,1);
            LcdWriteString(humidity);
        }

        if(ok_flag)
        {
            ok_flag = 0;
            ok_flag_humi = 1;
            menuCO2();
        }
    }
}


void menu(){

    sprintf(temperature, " %.1f", zeljena_temperatura);
    LcdSetCursor(1,1);
    LcdWriteString((char *)"Set air temp.   ");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString((char *)"         ");

    while(!ok_flag_humi)
    {
        CLRWDT();                           // Reset watchdog timer

        manage_buttons();

        if(plus_flag)
        {
            zeljena_temperatura += PODEOK_TEMP;
            if(zeljena_temperatura > 40) zeljena_temperatura = 40;  //zastita
            plus_flag = 0;

            sprintf(temperature, " %.1f", zeljena_temperatura);
            LcdSetCursor(2,1);
            LcdWriteString(temperature);
        }

        if(minus_flag)
        {
            zeljena_temperatura -= PODEOK_TEMP;
            if(zeljena_temperatura < 15) zeljena_temperatura = 15;  //zastita
            minus_flag = 0;

            sprintf(temperature, " %.1f", zeljena_temperatura);
            LcdSetCursor(2,1);
            LcdWriteString(temperature);
        }

        if(ok_flag)
        {
            ok_flag = 0;
            menuHumi();
        }
    }

    ok_flag = 0;
    ok_flag_humi = 0;
    ok_flag_co2 = 0;
}

//void slanjePodataka()
//{
//    uart_data[0] = '\0';
//    str[0] = '\0';
//    sprintf(str, "%.1f", temp);
//    strncat(uart_data, str, 5);
//    sprintf(str, "%.1f", humi);
//    strncat(uart_data, str, 5);
//    sprintf(str, "%.1f", t);
//    strncat(uart_data, str, 5);
//    sprintf(str, "%4d", co2);
//    strncat(uart_data, str, 5);
//
//    UARTWriteString(uart_data);
//}

void main(void) {

    LcdInit();
    initSHT71();
    resetDS18B20();
    adcInit();
    UARTInit(9600);
    IOPinsConfig(); //konfiguracija pinova sa tasterima i izlaza za pobudu releja
    IOPinsInit();   //inicijalizacija pinova sa tasterima i izlaza za pobudu releja
    resetValues();  //pocetne vrednosti promeljivih
    initTimer1();
    initTimer0();   // Used for WDT
    initWDT();      // Reset on ~35 sec
    __delay_ms(100);

    getResSHT71(&temp, &humi);  //pocetna merenja
    //co2 = measureTGS4161();     //preskacemo prvo merenje co2 jer senzor nije zagrejan
    t = getTempDS18B20();

    while(1)
    {
        if(measure)
        {
            measure = 0;

            switch(measure_count)
            {
                case 0:
                {
                    getResSHT71(&temp, &humi);
                    UARTWriteString((char *)"\nSHT OK.\n");
                }
                break;

                case 1:
                {
                    if(measure_co2)
                    {
                        co2 = measureTGS4161();
                        UARTWriteString((char *)"\nTGS OK.\n");
                    }
                }
                break;

                case 2:
                {
                    t = getTempDS18B20();
                    UARTWriteString((char *)"\nDS OK.\n");
                }
                break;

                default:
                    break;
            }

            if(++measure_count > 3) measure_count = 0;
        }

        if(disp)
        {
            disp = 0;

            switch(disp_count)
            {
                case 1:
                    displayAirTemp(temp);
                    break;
                case 2:
                    displayAirMoist(humi);
                    break;
                case 3:
                    displayCO2(co2);
                    break;
                case 4:
                    displaySoilTemp(t);
                    break;

                default:
                    break;
            }
            //slanjePodataka();
            disp_count++;
            if(disp_count > 4) disp_count = 0;
        }

        if (MENU)   //ako je pritisnut taster MENU menu_flag se postavlja na 1
        {
            __delay_ms(20);
            if(MENU) menu_flag = 1;
            while(MENU);
            __delay_ms(20);
        }
        if(menu_flag)
        {
            menu_flag = 0;
            PIE1bits.TMR1IE = 0;            // Disable TMR1 interrupts
            menu();
            PIE1bits.TMR1IE = 1;            // Enable TMR1 interrupts
        }

        regulacija();
        CLRWDT();                           // Reset watchdog timer
    }
}

