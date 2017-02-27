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
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
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
unsigned char tmr_count, measure , disp_count, disp, measure_co2;
unsigned char humidity[5]={0,0,0,0,0};
unsigned char temperature[5]={0,0,0,0,0};
char uart_data[30], str[5]; //za uart
float temp, humi, t, zeljena_temperatura, zeljena_vlaznost;
unsigned int co2, zeljena_co2, tmr_co2;

static void interrupt isr(){
    INTCONbits.T0IF = 0;    //interrupt flag clear
    //if(tmr_co2 == 5450) //priblizno 3 min (5450 = 180/0.033)
    //{
    //    tmr_co2 = 0;
    //    TGS_OFF = ~TGS_OFF;
    //}
    //if((!TGS_OFF) && (tmr_co2 > 3600))  //senzor je ukljucen duze od 2 minuta
    //{
    //    measure_co2 = 1;    //mogu se vrsiti merenja
    //}
    //else measure_co2 = 0;   // ne vrse se merenja

    if(tmr_count == 91) //priblizno 3s
    {
        tmr_count = 0;
        measure = 1;
    }
    else if(tmr_count == 65)  //priblizno 2s
    {
        disp = 1;
    }
    tmr_count++;    //obican brojac prekida
    tmr_co2++; //brojac prekida koji ce sluziti za odredj. vremena uklj. i isklj. senzora co2
    TMR0 = 0;
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
    LcdWriteString("Air temperature ");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString(" deg        ");
}

void displayAirMoist(float humi2){
    sprintf(humidity, " %.1f", humi2);
    LcdSetCursor(1,1);
    LcdWriteString("Air moist       ");
    LcdSetCursor(2,1);
    LcdWriteString(humidity);
    LcdSetCursor(2,6);
    LcdWriteString(" %          ");
}

void displayCO2(unsigned int co22){
    LcdSetCursor(1,1);
    LcdWriteString("CO2 in air      ");
    if(measure_co2)
    {
        LcdWriteInt(co22,2,1);
        LcdSetCursor(2,5);
        LcdWriteString(" ppm        ");
    }
    else
    {
        LcdSetCursor(2,1);
        LcdWriteString("Measuring...    ");
    }
}

void displaySoilTemp(float t2){
    sprintf(temperature, " %.1f", t2);
    LcdSetCursor(1,1);
    LcdWriteString("Soil temperature");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString(" deg        ");
}

void resetValues(){ //f-ja koja dodeljuje pocetne vrednosti promenljivima
    ok_flag = 0;
    menu_flag = 0;
    plus_flag = 0;
    minus_flag = 0;
    tmr_count = 0;
    measure = 0;
    disp_count = 0;
    disp = 1;
    zeljena_temperatura = 24;
    zeljena_vlaznost = 90;
    zeljena_co2 = 1000;
    ok_flag_humi = 0;
    ok_flag_co2 = 0;
    tmr_co2 = 0;
    measure_co2 = 1;
    co2 = 0;
}

void initTimer0(){  //f-ja koja konfigurise timer0, prekidi svakih 32,76ms
    OPTION_REGbits.T0CS = 0;    //timer 0 clock source, 0 znaci Fosc/4
    OPTION_REGbits.PSA = 0; //prescaler dodeljen timer0
    OPTION_REG |= 0b00000111;   //prescaler 1:256
    INTCONbits.GIE = 1; //globalni prekidi dozvoljeni
    TMR0 = 0;   //256*256*0,5us = prekid svakih 32,76ms
    INTCONbits.T0IE = 1;    //prekid timer0 dozvoljen
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

void menuCO2(){
    LcdSetCursor(1,1);
    LcdWriteString("Set CO2 in air  ");
    LcdSetCursor(2,1);
    LcdWriteString("          ");
    LcdSetCursor(2,1);
    LcdWriteInt(zeljena_co2,2,1);
    LcdSetCursor(2,6);
    LcdWriteString("        ");
    while(!ok_flag)
    {
    if(PLUS)
        {
            __delay_ms(20);
            if(PLUS) plus_flag = 1;
            while(PLUS);
            __delay_ms(20);
        }

    if(MINUS)
        {
            __delay_ms(20);
            if(MINUS) minus_flag = 1;
            while(MINUS);
            __delay_ms(20);
        }

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

    if(OK)    //debouncing
        {
            __delay_ms(20);
            if(OK)  ok_flag = 1;
            while(OK);
            __delay_ms(20);
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
    LcdWriteString("Set air moist   ");
    LcdSetCursor(2,1);
    LcdWriteString(humidity);
    LcdSetCursor(2,6);
    LcdWriteString("        ");

    while(!ok_flag_co2)
    {
        if(PLUS)
            {
                __delay_ms(20);
                if(PLUS) plus_flag = 1;
                while(PLUS);
                __delay_ms(20);
            }

        if(MINUS)
            {
                __delay_ms(20);
                if(MINUS) minus_flag = 1;
                while(MINUS);
                __delay_ms(20);
            }

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

        if(OK)    //debouncing
            {
                __delay_ms(20);
                if(OK)  ok_flag = 1;
                while(OK);
                __delay_ms(20);
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
    LcdWriteString("Set air temp.   ");
    LcdSetCursor(2,1);
    LcdWriteString(temperature);
    LcdSetCursor(2,6);
    LcdWriteString("         ");

    while(!ok_flag_humi)
    {
    if(PLUS)
        {
            __delay_ms(20);
            if(PLUS) plus_flag = 1;
            while(PLUS);
            __delay_ms(20);
        }

    if(MINUS)
        {
            __delay_ms(20);
            if(MINUS) minus_flag = 1;
            while(MINUS);
            __delay_ms(20);
        }

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

    if(OK)    //debouncing
        {
            __delay_ms(20);
            if(OK)    ok_flag = 1;
            while(OK);
            __delay_ms(20);
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

void slanjePodataka()
{
    uart_data[0] = '\0';
    str[0] = '\0';
    sprintf(str, "%.1f", temp);
    strncat(uart_data, str, 5);
    sprintf(str, "%.1f", humi);
    strncat(uart_data, str, 5);
    sprintf(str, "%.1f", t);
    strncat(uart_data, str, 5);
    sprintf(str, "%4d", co2);
    strncat(uart_data, str, 5);

    UARTWriteString(uart_data);
}

void main(void) {

    LcdInit();
    initSHT71();
    resetDS18B20();
    adcInit();
    UARTInit(9600);
    IOPinsConfig(); //konfiguracija pinova sa tasterima i izlaza za pobudu releja
    IOPinsInit();   //inicijalizacija pinova sa tasterima i izlaza za pobudu releja
    resetValues();  //pocetne vrednosti promeljivih
    initTimer0();

    getResSHT71(&temp, &humi);  //pocetna merenja
//    co2 = measureTGS4161(); preskacemo prvo merenje co2 jer senzor nije zagrejan
    t = getTempDS18B20();

    while(1)
    {
        if(measure)
        {
            measure = 0;
            getResSHT71(&temp, &humi);

            //if(measure_co2) //ako je senzor dovoljno zagrejan vrse se merenja
            //{
                co2 = measureTGS4161();
            //}
            
            t = getTempDS18B20();
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
            INTCONbits.T0IE = 0;    //prekid timer0 zabranjen
            menu();
            INTCONbits.T0IE = 1;    //prekid timer0 dozvoljen
        }

        regulacija();
    }
}

