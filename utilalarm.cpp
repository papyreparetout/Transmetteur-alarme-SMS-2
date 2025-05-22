
#include <Arduino.h>
#include "utilalarm.h"
#include "Adafruit_FONA.h"

void DecodeAlarm( char* msgbuf, int msgbufLen, char* dispchar) 
// sous programme de decodage du message envoye par la centrale 
{
  // String teststr;
  byte bufix=0;
  const int DISP_BUF_LEN=16+1+2;
  byte dispBuffer[DISP_BUF_LEN + 1]="Not Connected";
  const char allmonths[] = {"JANFEVMARAVRMAIJUNJULAOUSEPOCTNOVDEC"};
  const char alldays[] = {"DIMLUNMARMERJEUVENSAM"};
 //Variable pour voir si changement dans l'affichage 
//  static int previousCS =-1;
//  bool bScreenHasUpdated =false;
  int ixMsgbuf=2;//skip 2 header bytes
 	msgbufLen= msgbufLen - 2; //remove checksum and zero terminator
// Serial.println(msgbuf);
// Serial.println(msgbufLen);
 // decodage du message envoye par la centrale
  while(ixMsgbuf <= msgbufLen)
  {
    uint8_t rx = msgbuf[ixMsgbuf++];
    if (rx>=0 && rx < 0x0f)
    {//not implemented
    }
    else if (rx == 0x13)
    {//not implemented
    }
    else if (rx == 0x1b)
    {//to do with foreign character set - not implemented
    }
    else if (rx>= 0x20 && rx <= 0x7F)
    {//Normal ASCII
      if (bufix==0)
        //Force Screen clear at start of each message
        for(int m=0;m<DISP_BUF_LEN;m++)
          dispBuffer[m]=' ';

      if (bufix < DISP_BUF_LEN)
        dispBuffer[bufix++]=(char)rx;
    }
    else if (rx>= 0x80 && rx <= 0x8F)
    {//Date in encoded format
      int b0=rx;
      int b1=msgbuf[ixMsgbuf++];
      int b2=msgbuf[ixMsgbuf++];
      int b3=msgbuf[ixMsgbuf++];

      byte nMonth= (b0 & 0x0f)-1;
      byte jour = (b1 & (128+64+32))>> 5;
      byte date = (b1 & (31));
      byte h1=(b2 & 0xf0)>>4; if(h1==0x0A) h1=0;
      byte h2=(b2 & 0x0f); if(h2==0x0A) h2=0;
      byte m1=(b3 & 0xf0)>>4; if(m1==0x0A) m1=0;
      byte m2=(b3 & 0x0f); if(m2==0x0A) m2=0;

      memcpy(dispBuffer+0,alldays+(jour*3),3);
      dispBuffer[3]=' ';
      dispBuffer[4]=('0'+(int)(date/10));
      dispBuffer[5]=('0'+(date%10));
      dispBuffer[6]=' ';

      memcpy(dispBuffer+7,allmonths+(nMonth*3),3);
      dispBuffer[10]=' ';
      dispBuffer[11]='0'+h1;
      dispBuffer[12]='0'+h2;
      //if (dateFlash)
      dispBuffer[13]= ':';
      //else
      //  buffer[13]= F(' ');
      //dateFlash=!dateFlash;
      //buffer[13]= ((millis()/500)&1) ==0? ':':' ';
      dispBuffer[14]='0'+m1;
      dispBuffer[15]='0'+m2;
      bufix=0;
    }
    else if (rx == 0x90)
    {//CLS
      bufix=0;
      for(int m=0;m<DISP_BUF_LEN;m++)
        dispBuffer[m]=' ';
    }
    else if (rx == 0x91)
    {//HOME
      bufix=0;
    }
    else if (rx >= 0xA0 && rx <= 0xAf)
    {//MOVE cursor to position x
      bufix = (rx & 0x0f); //-1 gives us 2 *'s  but without -1 we go off screen at Login ***
    }
    else if (rx >= 0xB0 && rx <= 0xBF)
    {//{BLINK_N}" Bxh Blink x chars starting at current cursor position
     //not implementing this as it will cause unnecessary traffic sending display each second
      //int nChars = (rx & 0x0f)-1;
      //if (dateFlash)
      //  buffer[i]= ':';
      //else
      //  buffer[i]= ' ';
      //dateFlash=!dateFlash;
    }
    else if (rx >= 0xC0 && rx <= 0xCf)
    {// Set position to x and clear all chars to right
      int i = (rx & 0x0f);
      if (i < DISP_BUF_LEN)
        bufix = i;
      for(int n=bufix;n<DISP_BUF_LEN;n++)
        dispBuffer[bufix++]=' ';
    }
    else if (rx>= 0xE0 && rx <= 0xFF)
    {// Special Characters Arrows and foreign chars
      int i = (rx & 0x0f);

      char c=0;
      if (i ==3)  c= 'e';
      if (i==4) c= '*';
      else if (i==5)  c= '#';
      else if (i==7)  c= '>';

      if (c>0)
        if (bufix < DISP_BUF_LEN)
          dispBuffer[bufix++]=c;
    }
    else
    {//unknown command
      Serial.println("{"+String(rx)+"}");
    }

    //Note: there are quite a few codes in Engineer menu to deal with flashing cursors and characters - cannot do easily in html
  }
// controle de checksum 
//  Serial.println(msgbuf);
		char cs = 0;
		for(int n=0;n<(msgbufLen+1);n++)
		{
			char rx = msgbuf[n];
		//	if (n<(msgbufLen+1)) //dont sum cs or terminator
				cs+=rx;
		}

		if (cs == 0)
			cs++; //protocol avoids 0 except for end marker- so will send cs 00 as 01
		if (cs != msgbuf[msgbufLen+1])
		{
      String csfails = "";
      for(int n=0;n<(DISP_BUF_LEN+1);n++) csfails= csfails +(char)dispBuffer[n];
			csfails = csfails + char(0x00);
			Serial.println("CS Fail calcul :"+ String(cs) + " lu: " + String(msgbuf[msgbufLen+1]) + " buffer: " + csfails);
			return ;
		}

// Detection LED défaut et LED Alarme
	bool bIsPanelWarning = (msgbuf[1] & 0x04) != 0;
	bool bIsPanelAlarm = (msgbuf[1] & 0x02) != 0;
 
  // message d'alerte ou d'alarme
  dispBuffer[16]='|'; //this may overwrite a char sometimes...ok.
  //  x = (expression)? valeur_x_si_expression_vraie : valeur_x_si_expression_fausse
  dispBuffer[17]=(bIsPanelAlarm)?'A':' ';
  dispBuffer[18]=(bIsPanelWarning)?'W':' ';

  for(int n=0;n<(DISP_BUF_LEN+1);n++) dispchar[n] = (char)dispBuffer[n];
	return ;
}

void flushSerial() {
  while (Serial.available())
    Serial.read();
}
