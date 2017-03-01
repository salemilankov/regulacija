#include "sht71.h"
#include "lcd.h"

unsigned int tempervalue[2]={0,0};   //globalne prom koje ce se koristiti u f-ji measure i calc
typedef union
{ unsigned int i;
  float f;
} value;

void initSHT71(){
    TRISBbits.TRISB7 = 0;   //output
    TX_DATA;
}

void tranStartSHT71(){
    TX_DATA;
    DATA = 1;
    SCK = 0;
    __delay_us(2);  //cekanja su proizvoljna
    SCK = 1;
    __delay_us(2);
    DATA = 0;
    __delay_us(1);
    SCK = 0;
    __delay_us(2);
    SCK = 1;
    __delay_us(2);
    DATA = 1;
    __delay_us(1);
    SCK = 0;
    __delay_us(2);
    DATA = 0;
    __delay_us(2);
}

//float measureHumiditySHT71(float temp){
//    unsigned short resL, resH;
//    unsigned res;
//    float humidity_lin, humidity_real;
//    TX_DATA;
//    SCK = 0;
//    DATA = 0;
//
//    tranStartSHT71();   //pocetak prenosa
//    writeByteSHT71(MEASURE_HUMI);   //slanje komande za merenje vlaznosti
//    __delay_ms(80); //vreme formiranja 12-bitnog rezultata
//
//    resH = readByteSHT71(); //1. bajt rezultata(iako je 12-bitni primamo 16 bita)
//
//    SCK = 1;
//    __delay_us(2);  //cekamo ack da prodje
//    SCK = 0;
//
//    resL = readByteSHT71(); //2.bajt rezultata
//    TX_DATA;
//    DATA = 1;
//    res = ((unsigned int)resH << 8) + (unsigned int)resL;   //obradjujemo primljeni rezultat
//
//    humidity_lin = C1 + C2*(float)res + C3*(float)res*(float)res;   //po datasheet-u
//    humidity_real = (temp - 25)*(T1 + T2*(float)res) + humidity_lin;   //po datasheet-u
//    return humidity_real;
//}
//
//float measureTemperatureSHT71(){
//    unsigned short resL, resH;
//    unsigned res;
//    float temperature;
//    TX_DATA;
//    SCK = 0;
//    DATA = 0;
//
//    tranStartSHT71();   //pocetak prenosa
//    writeByteSHT71(MEASURE_TEMP);   //slanje komande za merenje vlaznosti
//    __delay_ms(320); //vreme formiranja 12-bitnog rezultata
//
//    resH = readByteSHT71(); //1. bajt rezultata(iako je 12-bitni primamo 16 bita)
//
//    SCK = 1;
//    __delay_us(2);  //cekamo ack da prodje
//    SCK = 0;
//
//    resL = readByteSHT71(); //2.bajt rezultata
//    TX_DATA;
//    DATA = 1;
//    res = ((unsigned int)resH << 8) + (unsigned int)resL;   //obrada primljenog rezultata
//
//    temperature = D1 + D2*(float)res;   //po datasheet-u
//    return temperature;
//}

char measureSHT71(unsigned char *p_value, unsigned char *p_checksum, unsigned char mode){
    unsigned error = 0;
    unsigned int temp = 0;
    unsigned char loop_cnt=0;

    tranStartSHT71();                   //transmission start
    switch(mode){                     //send command to sensor
        case TEM        : error += writeByteSHT71(MEASURE_TEMP); break;    //merenje temperature
        case HUM        : error += writeByteSHT71(MEASURE_HUMI); break;    //merenje vlaznosti
        default     : break;
        }
    RX_DATA;    // DATA is input
    while (1)
    {
        if((DATA == 0) || (++loop_cnt > 200)) break; //wait until sensor has finished the measurement
        __delay_ms(10);
    }
    if(DATA) error += 1;                // or timeout (~2 sec.) is reached
    switch(mode){                     //send command to sensor
    case TEM         : temp = 0;                    //primanje rezultata merenja
                       temp = readByteSHT71(ACK);
                       temp <<= 8;
                       tempervalue[0] = temp;
                       temp = 0;
                       temp = readByteSHT71(ACK);
                       tempervalue[0] |= temp;
                        break;
    case HUM         : temp = 0;
                       temp = readByteSHT71(ACK);
                       temp <<= 8;
                       tempervalue[1] = temp;
                       temp = 0;
                       temp = readByteSHT71(ACK);
                       tempervalue[1] |= temp;
                        break;
    default     : break;
  }
  *p_checksum = readByteSHT71(noACK);  //read checksum
  return error;
}

float calcSHT71(float p_humidity ,float *p_temperature){
    float rh_lin;                     // rh_lin:  Humidity linear
    float rh_true;                    // rh_true: Temperature compensated humidity
    float t = *p_temperature;           // t:       Temperature [Ticks] 14 Bit
    float rh = p_humidity;             // rh:      Humidity [Ticks] 12 Bit
    float t_C;                        // t_C   :  Temperature [?]

    t_C = t*D2 + D1 - TEMP_GRESKA;                  //calc. temperature from ticks to [?]

    rh_lin = C3*rh*rh + C2*rh + C1;     //calc. humidity from ticks to [%RH]
    rh_true = (t_C - 25)*(T1 + T2*rh) + rh_lin;   //calc. temperature compensated humidity [%RH]

    if(rh_true > 99) rh_true = 99;       //cut if the value is outside of
    if(rh_true < 0.1) rh_true = 0.1;       //the physical possible range

    *p_temperature = t_C;               //return temperature [?]
    return rh_true;
}

char readByteSHT71(unsigned char ack){   //preuzeta od electronics base
    unsigned char i, res=0;
    RX_DATA;    // DATA is Input
    for (i=0x80;i>0;i/=2)             //shift bit for masking
    {
        SCK = 1;                          //clk for SENSI-BUS
        __delay_us(2);
        if (DATA) res = (res | i);        //read bit
        __delay_us(2);
        SCK = 0;
    }
    TX_DATA; // DATA is Output
    DATA = !ack;        // pull down DATA-Line
    SCK = 1;                //clk #9 for ack
    __delay_us(5);          //pulswith approx. 5 us
    SCK =   0;
    DATA = 1;           //release DATA-line  //ADD BY LUBING - ne menja nista
    return res;
}

char writeByteSHT71(unsigned char cmd){ //preuzeta od electronics base
    unsigned char i,error = 0;
    TX_DATA;

    for (i=0x80;i>0;i/=2) //shift bit for masking
    {
        if (i & cmd)
        DATA = 1; //masking value with i , write to SENSI-BUS
        else DATA = 0;
        SCK = 1;      //clk for SENSI-BUS
        __delay_us(5); //pulswith approx. 5 us
        SCK = 0;
    }
    DATA = 1;            //release dataline
    RX_DATA;    //primamo podatak
    SCK = 1;                //clk #9 for ack
    __delay_us(2);
    error = DATA;       //check ack (DATA will be pulled down by SHT11)
    __delay_us(2);
    SCK = 0;
    return error;       //error=1 in case of no acknowledge
}

void connectionResetSHT71(){
  unsigned char i;
  TX_DATA;    // DATA is output
  DATA = 1;
  SCK=0;                    //Initial state
  for(i=0;i<9;i++)                  //9 SCK cycles
  { SCK = 1;
    __delay_us(1);
    SCK = 0;
    __delay_us(1);
  }
  tranStartSHT71();                   //transmission start
  RX_DATA;    // DATA is Input
}

char softReset(){
  unsigned char error=0;
  connectionResetSHT71();              //reset communication
  error += writeByteSHT71(RESET_SHT71);       //send RESET-command to sensor
  return error;                     //error=1 in case of no response form the sensor
}

char readStatusRegSHT71(unsigned char *p_value, unsigned char *p_checksum)
{
  unsigned char error=0;
  tranStartSHT71();                   //transmission start
  error = writeByteSHT71(STATUS_REG_R); //send command to sensor
  *p_value = readByteSHT71(ACK);        //read status register (8-bit)
  *p_checksum = readByteSHT71(noACK);   //read checksum (8-bit)
  return error;                     //error=1 in case of no response form the sensor
}

void getResSHT71(float *p_temp, float *p_humi){   //f-ja koja vraca rezultate u float formatu
    value humi_val,temp_val;
    unsigned char error, checksum;
    char inp;
        error=0;
        error += measureSHT71((unsigned char*) (&humi_val.i),&checksum,HUM);  //measure humidity
        error += measureSHT71((unsigned char*) (&temp_val.i),&checksum,TEM);  //measure temperature
        error += readStatusRegSHT71(&inp, &checksum);
        if(error != 0)
        {
            connectionResetSHT71();                 //in case of an error: connection reset
            LcdSetCursor(1,1);
            LcdWriteString("Greska SHT71!");
            __delay_ms(1000);
        }
        else
        {
            humi_val.f = (float)tempervalue[1];                   //converts integer to float
            temp_val.f = (float)tempervalue[0];                   //converts integer to float
            humi_val.f = calcSHT71(humi_val.f,&temp_val.f);      //calculate humidity, temperature
            *p_temp = temp_val.f;
            *p_humi = humi_val.f;
        }
//        return humi_val.f;
}

