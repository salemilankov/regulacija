
#include "lcd.h"


//LCD Functions Developed by electroSome


void LcdPort(char a)
{
	if(a & 1)
		D4 = 1;
	else
		D4 = 0;

	if(a & 2)
		D5 = 1;
	else
		D5 = 0;

	if(a & 4)
		D6 = 1;
	else
		D6 = 0;

	if(a & 8)
		D7 = 1;
	else
		D7 = 0;
}
void LcdCmd(char a)
{
	RS = 0;             // => RS = 0
	LcdPort(a);
	EN  = 1;             // => E = 1
        __delay_ms(4);
        EN  = 0;             // => E = 0
}

LcdClear()
{
	LcdCmd(0);
	LcdCmd(1);
}

void LcdSetCursor(char a, char b)
{
	char temp,z,y;
	if(a == 1)
	{
	  temp = 0x80 + b - 1;
		z = temp>>4;
		y = temp & 0x0F;
		LcdCmd(z);
		LcdCmd(y);
	}
	else if(a == 2)
	{
		temp = 0xC0 + b - 1;
		z = temp>>4;
		y = temp & 0x0F;
		LcdCmd(z);
		LcdCmd(y);
	}
}

void LcdInit()
{
  // ovo sam ja dodao   ////////////////////////////////
  OPTION_REG |= 0b10000000;//iskljucujemo pull-up otpornike porta B
  ANSELH &= 0b11000000;  //pinovi koji se koriste za LCD su digitalni
  TRISB = 0b00000000; //ceo port b je izlazni
  /////////////////////////////////////////////////////
  LcdPort(0x00);
   __delay_ms(20);
  LcdCmd(0x03);
	__delay_ms(5);
  LcdCmd(0x03);
	__delay_ms(11);
  LcdCmd(0x03);
  /////////////////////////////////////////////////////
  LcdCmd(0x02);
  LcdCmd(0x02);
  LcdCmd(0x08);
  LcdCmd(0x00);
  LcdCmd(0x0C);
  LcdCmd(0x00);
  LcdCmd(0x06);
}

void LcdWriteChar(char a)
{
   char temp,y;
   temp = a&0x0F;
   y = a&0xF0;
   RS = 1;             // => RS = 1
   LcdPort(y>>4);             //Data transfer
   EN = 1;
   __delay_us(40);
   EN = 0;
   LcdPort(temp);
   EN = 1;
   __delay_us(40);
   EN = 0;
}

void LcdWriteString(char *a)
{
	int i;
	for(i=0;a[i]!='\0';i++)
	   LcdWriteChar(a[i]);
}

void LcdShiftRight()
{
	LcdCmd(0x01);
	LcdCmd(0x0C);
}

void LcdShiftLeft()
{
	LcdCmd(0x01);
	LcdCmd(0x08);
}

void LcdWriteInt(unsigned int i, char row, char column){    //moja f-ja za ispis int promenjive na dislpej
    unsigned char ch, ch_manje_1000=0, ch_manje_100=0;

    ch = i/1000;
    if(ch == 0)
    {
        ch_manje_1000 = 1;
        LcdSetCursor(row,column);
        LcdWriteChar(' ');
    }
    else
    {
        LcdSetCursor(2,1);
        LcdWriteChar(ch+'0');
    }


    i = (i - ch*1000);
    ch = i/100;
    if((ch == 0) && (ch_manje_1000))
    {
        ch_manje_100 = 1;
        LcdSetCursor(row,column+1);
        LcdWriteChar(' ');
    }
    else
    {
        LcdSetCursor(2,2);
        LcdWriteChar(ch+'0');
    }


    i = (i - ch*100);
    ch = i/10;
    if((ch == 0) && (ch_manje_1000) && (ch_manje_100))
    {
        LcdSetCursor(row,column+2);
        LcdWriteChar(' ');
    }
    else
    {
        LcdSetCursor(2,3);
        LcdWriteChar(ch+'0');
    }


    i = (i - ch*10);
    ch = i;
    LcdSetCursor(row,column+3);
    LcdWriteChar(ch+'0');
}