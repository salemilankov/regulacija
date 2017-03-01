#include "ds18b20.h"

char resetDS18B20(){
    PIN_DIR_18B20 = 0;   // Tris = 0 (output)
    PIN_18B20 = 0;     // set pin# to low (0)
    __delay_us(480);    //1 wire require time delay
    PIN_DIR_18B20 = 1;   // Tris = 1 (input)
    __delay_us(60);  // 1 wire require time delay

    if (PIN_18B20 == 0)
    {
        __delay_us(480);
        return 0;   // return 0 ( 1-wire is presence)
    }
        else
        {
            __delay_us(480);
            return 1;   // return 1 ( 1-wire is NOT presence)
        }
}


void writeDS18B20 (char Cmd){
    char i;
    PIN_DIR_18B20 = 1;   // Tris = 1 (input)
    for(i = 0; i < 8; i++)
    {
        if((Cmd & (1<<i))!= 0)
        {
            PIN_DIR_18B20 = 0;   // Tris = 0 (output)
            PIN_18B20 = 0; // set pin# to low (0)
            __delay_us(1); // 1 wire require time delay
            PIN_DIR_18B20 = 1;   // Tris = 1 (input)
            __delay_us(60); // 1 wire require time delay
         }
            else
            {
                    //write 0
                PIN_DIR_18B20 = 0;   // Tris = 0 (output)
                PIN_18B20 = 0; // set pin# to low (0)
                __delay_us(60); // 1 wire require time delay
                PIN_DIR_18B20 = 1;   // Tris = 1 (input)
            }
    }
}


char readDS18B20 (){
    char i,result = 0;
    PIN_DIR_18B20 = 1;   // Tris = 1 (input)

    for(i = 0; i < 8; i++)
    {
        PIN_DIR_18B20 = 0;   // Tris = 0 (output)
        PIN_18B20 = 0; // genarate low pluse for 2us
        __delay_us(2);
        PIN_DIR_18B20 = 1;   // Tris = 1 (input)

        if(PIN_18B20 != 0) result |= 1<<i;

        __delay_us(60); // wait for recovery time
    }
return result;
}


float getTempDS18B20(){
    unsigned temp;
    unsigned short tempL, tempH;
    float temperatura;
    if(!resetDS18B20())
    {
        writeDS18B20(SKIP_ROM);
        writeDS18B20(CONVERT_T);
        __delay_ms(750);

        resetDS18B20();
        writeDS18B20(SKIP_ROM);
        writeDS18B20(READ_SCRATCHPAD);

        tempL = readDS18B20();
        tempH = readDS18B20();

        temp = ((unsigned int)tempH << 8) + (unsigned int)tempL;
        temperatura = (float)temp * 0.0625 - TEMP_GRESKA_DS;
    }
    return temperatura;
}