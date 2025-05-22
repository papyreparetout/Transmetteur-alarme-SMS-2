#ifndef utilalarm_h
#define utilalarm_h
#include <Arduino.h>
#include "Adafruit_FONA.h"

// sous programme de decodage du message envoye par la centrale
void DecodeAlarm( char* msgbuf, int msgbufLen, char * dispchar ); 

// uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout);

void flushSerial();

// sous programme d'envoi du sms
// void sendSMS(char* sendto, char* message, char* PIN, Adafruit_FONA &fona);

#endif
