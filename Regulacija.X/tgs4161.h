/* 
 * File:   tgs4161.h
 * Author: Milankov
 *
 * Created on 06.08.2015., 11.14
 */

#include<xc.h>

#define _XTAL_FREQ 8000000

#define V350 230   //napon na izlazu senzora (u mV) pri 350ppm
#define GAIN 5.7 //pojacanje neinv. pojacavaca
#define KADC 204.8  //nagib prave AD konvertora
#define N1 6650
#define N2 13500
void adcInit(); //inicijalizacija AD konvertora
unsigned int adcRead(); //f-ja koja vraca 10-bitni rezultat AD konverzije
unsigned int measureTGS4161();  //f-ja koja vraca kolicinu CO2 u ppm