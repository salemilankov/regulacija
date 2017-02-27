/* 
 * File:   lcd.h
 * Author: Milankov
 *Ovaj header fajl u paketu sa lcd.c se koristi za rad sa lcd  2x16 displejom
 * Created on 29.07.2015., 11.35
 */



#include<xc.h>

#define _XTAL_FREQ 8000000

#define RS RB5  //RB4 za RS kada radim sa easypic6, na diplomskom RB5
#define EN RB4
#define D4 RB0
#define D5 RB1
#define D6 RB2
#define D7 RB3

void LcdPort(char a);
void LcdCmd(char a);
void LcdSetCursor(char a, char b);  //a je red, b kolona
void LcdInit(); //inicijalizuje pinove kontorlera i LCD
void LcdWriteChar(char a);  //ispisuje karakter
void LcdWriteString(char *a);   //ispisuje string (karakter po karakter)
void LcdShiftRight();   //siftuje sadrzaj ekrana udesno (oba reda)
void LcdShiftLeft();    //                       ulevo
void LcdWriteInt(unsigned int i, char row, char column);    //f-ja koja ispisuje ceo broj koji je u rasponu od 0-9999
                                                            //row i column oznacavaju mesta na displeju gde pocinje ispis