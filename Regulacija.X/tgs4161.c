#include "tgs4161.h"


void adcInit(){
    TRISA |= 0b00000001;    //RA0 ulazni pin
    ANSEL |= 0b00000001;    //RA0 je analogni pin
    ADCON0 = 0b00000000;
    ADCON1 |= 0b10000000;
    PIE1bits.ADIE = 0;  //zabrana prekida AD konvertora
}

unsigned int adcRead(){
    unsigned int temp = 0;
    unsigned short tempL=0, tempH=0;

    ADCON0bits.ADON = 1;    //AD enable
    __delay_ms(1);  //cekamo da prodje aquisition time
    ADCON0bits.GO = 1;  //pokrecemo AD konverziju

    while(ADCON0bits.GO);   //cekamo da se zavrsi AD konverzija

    tempL |= ADRESL;    //uzimamo rezultat
    tempH |= ADRESH;

    temp = ((unsigned int)tempH <<8) + (unsigned int)tempL;     //konverzija 10-bitnog rezultata u celobrojnu vrednost

    return temp;
}

unsigned int measureTGS4161(){
    unsigned int temp=0, ppm=0, ppm_temp=0, krez1, krez2;
    char i;

    krez1 = -23.43;
    krez2 = -53.63;

    INTCONbits.T0IE = 0;    //zabrana prekida timer0
    for(i=0;i<7;i++)
    {
        temp = adcRead();
        
        if(temp > 236) ppm = krez1*temp + N1;
        else ppm = krez2*temp + N2;
        ppm_temp += ppm;
        __delay_ms(20);
    }
    INTCONbits.T0IE = 1;    //prekid timer0 dozvoljen

    ppm = ppm_temp/7;
    if(ppm < 350) ppm = 350;
    if(ppm > 5500) ppm = 5500;
    
    return ppm;
}