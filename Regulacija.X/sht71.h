/* 
 * File:   sht71.h
 * Author: Milankov
 *
 * Created on 30.07.2015., 21.09
 */

#include <xc.h>
#define _XTAL_FREQ 8000000

#define SCK RB6
#define DATA RB7
#define RX_DATA TRISBbits.TRISB7 = 1
#define TX_DATA TRISBbits.TRISB7 = 0

//komande
#define STATUS_REG_W 0x06
#define STATUS_REG_R 0x07
#define MEASURE_TEMP 0x03
#define MEASURE_HUMI 0x05
#define RESET_SHT71  0x1E
#define ACK 1
#define noACK 0
#define TEM 2
#define HUM 3

//iz nekog razloga se ne koriste konstante date u datasheet-u, nego one preuzete sa electronics-base.com
#define C1 -4.0//-2.0468  //konstante za proracun Rh, za 12-bitno merenje
#define C2 +0.0405 //0.0367
#define C3 -0.0000028//-0.00000015955

//konstante iz datasheet-a
#define D1 -40.1
#define D2 0.01 //T = d1 + d2*SOt
#define TEMP_GRESKA 1 //greska pri merenju temperature, oduzima se od rezultata

#define T1 +0.01             // for 14 Bit @ 5V
#define T2 +0.00008    //za racunanje realne vlaznosti(zavisi od temperature)

void tranStartSHT71();  //slanje komande za pocetak prenosa
char readByteSHT71(unsigned char);  //citanje 1 bajta iz senzora, argument ack ili noack
char writeByteSHT71(unsigned char); //slanje 1 bajta u senzor, argument ack ili noack
void initSHT71();   //inicijalizacija senzora
void connectionResetSHT71();    //resetovanje konekcije
char softReset();   //soft reset senzora, koristi se u connectionReset f-ji
char readStatusRegSHT71(unsigned char *p_value, unsigned char *p_checksum); //citanje statusnog registra senzora
char measureSHT71(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode);   //vrsenje merenja u zavisnosti od argumenta mode
float calcSHT71(float p_humidity ,float *p_temperature);    //proracun T i Rh na osnovu merenja
void getResSHT71(float *p_temp, float *p_humi);   //f-ja koja vraca vrednost T i Rh u float formatu