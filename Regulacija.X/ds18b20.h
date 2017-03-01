/* 
 * File:   ds18b20.h
 * Author: Milankov
 *
 * Created on 30.07.2015., 14.29
 */

#define _XTAL_FREQ 8000000
#include <xc.h>

#define SKIP_ROM 0xCC
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xBE

#define TEMP_GRESKA_DS 0.3

#define PIN_18B20 RC5
#define PIN_DIR_18B20 TRISCbits.TRISC5


char resetDS18B20();  //proziva senzor, vraca 0 ukoliko je prisutan
void writeDS18B20(char Cmd);  //salje komandu senzoru
char readDS18B20();   //prima podatke od senzora, f-ja vraca 8-bitni rezultat
float getTempDS18B20();    //moja funkcija, vrsi merenje temperature i vraca float rezultat