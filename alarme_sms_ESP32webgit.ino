/*
Recepteur de trame d'une alarme Aritech CD34 et decodage pour envoi SMS
lorsque l alarme a detecte des situations pre definiestaire du SMS
Version pour carte TTGO T Call avec ESP32 et possibilité de changement du numéro de téléphone destinataire du SMS
changement (inversion) des pins pour la connexion série avec l'alarme (pin 14 en réception au lieu 12 avant) afin de 
ne pas être géné par la carte fille pour le flashage programme (libération pin 12)
*/
#include "utilalarm.h"
#include "Adafruit_FONA.h"
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

// TTGO T-Call pin definitions
#define MODEM_RST            5
#define MODEM_PWKEY          4
#define MODEM_POWER_ON       23
#define MODEM_TX             27
#define MODEM_RX             26
#define I2C_SDA              21
#define I2C_SCL              22

#define FONA_RST 5

// HardwareSerial Serial2(2);
HardwareSerial *fonaSerial = &Serial2;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);
uint8_t type;
unsigned long milliprec;  // pour test regulier

// numero de tel destinataire du SMS
char sendto[21] = "+336xxxxx";  // numero 1
char sendto1[21] = "+336xxxxxxx";  // numero 2
char sendto2[21] = "+336xxxxxxx";  // numero 3

char PIN[5] = "xxxx";  // code PIN de la carte telephone du module SIM8000
char message[30];
String messtest = "Alarme test envoi SMS";

// variables pour le dialogue avec l alarme
byte endFrame = 0x00; // indicateur de début et fin de trame
const int ixMaxPanel = 40; // doit etre const pour pouvoir dimensionner un tableau
int msgbufLen = 0;
char msgbuf[ixMaxPanel]; // longueur doit etre egale à ixMaxPanel
char charIn; // caractere recu
char lastcharIn = 0xFF; // caractere recu precedent
bool mbIsPanelWarning=false;
bool mbIsPanelAlarm=false;
int RKPID = 0;   // indice de la console receptrice des messages
// pour test
bool verbose = true;  // pour suivi du process

//
//  Création d'un serveur web pour la modification du numéro de téléphone destinataire du SMS
//
AsyncWebServer server(80);

// REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "xxxxxxxx";
const char* password = "xxxxxxxxx";

const char* PARAM_INPUT_1 = "input1";
String entree="+336xxxxxxx";  // numero par defaut

// HTML web page pour la demande de numéro
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Numero telephone SMS</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script>
    function submitMessage() {
      alert("Numero remplace");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script>
  </head><body>
  <h2>Numero telephone destinataire SMS</h2>
   <form action="/get" target="hidden-form">
    Numero tel SMS (current value %input1%): <input type="text" name="input1">
    <input type="submit" value="Entree" onclick="submitMessage()">
   </form>
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// Remplace la valeur initiale par la valeur entree
// ATTENTION variable globale entree 
String processor(const String& var){
  //Serial.println(var);
  if(var == "input1"){
    return entree;
  }
  return String();
}
//
//
//
void setup()
{
	//while (!Serial);
	Serial.begin(115200);
	if (verbose) { Serial.println(F("FONA basic test"));
		Serial.println(F("Initializing....(May take 3 seconds)"));
		}
// Interface avec le SIM800, definition des pins de connection
// Set-up modem reset, enable, power pins
	pinMode(MODEM_PWKEY, OUTPUT);
	pinMode(MODEM_RST, OUTPUT);
	pinMode(MODEM_POWER_ON, OUTPUT);

	digitalWrite(MODEM_PWKEY, LOW);
	digitalWrite(MODEM_RST, HIGH);
	digitalWrite(MODEM_POWER_ON, HIGH);
// Interface avec le SIM800, definition des pins de connection
	fonaSerial -> begin(9600,SERIAL_8N1, MODEM_RX, MODEM_TX);

	if (! fona.begin(*fonaSerial)) {
		Serial.println(F("SIM800 non trouve"));
		while (1);
		}
	if (verbose) { Serial.println(F("SIM800 OK"));
				}
	int RXD1 = 14;
	int TXD1 = 12;
	Serial1.begin(1953, SERIAL_8N1, RXD1, TXD1);  // communication avec l'alarme 

	if (verbose) { Serial.println(" code pin carte ");
		Serial.println(PIN);
		Serial.println(F(" Unlocking SIM card: "));
		}
	if (! fona.unlockSIM(PIN)) {
		Serial.println(F("Failed"));
	} 
	else 
	{
		Serial.println(F("PIN OK!"));
	}
	delay(5000);
	milliprec= millis();
	Serial.println("fin d'initialisation SIM800");
	
	  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      entree = inputMessage;
      entree.toCharArray(sendto,21);
      Serial.println("Numero SMS change"+entree);
 // envoi d'un sms au nouveau numéro choisi
	  //messtest.toCharArray(message,30);
      sendSMS(sendto, "Alarme Roguin test envoi SMS");
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
  });
    server.onNotFound(notFound);
  server.begin();
  
 // Envoi d'un SMS de test chaque fois que l'on reinitialise l'alarme
 // messtest.toCharArray(message,30);
  sendSMS(sendto, "Alarme test initialisation SMS");
}

void loop() 
{	
 const int dispBufferlen =20;  // doit etre identique à la valeur dans le sous programme de decodage
 char retour[dispBufferlen];
 String retext = "";
// static String retextprec = "";
// String messfin = "Fin defaut ou alarme";
static bool alarm = false;

// test regulier par envoi d un SMS tous les xx millisecondes 
  unsigned long period = 86398357; // 1 jour en ms - avec valeur adaptee empiriquement pour dérive >0 de environ 1,6s par jour
	  if ((millis()-milliprec)> period) 
			{
			messtest.toCharArray(message,30);
		    sendSMS(sendto, "Alarme test envoi SMS");
            milliprec = millis();
	  		}
	  if (Serial1.available()) 
	  { charIn = Serial1.read();
		if(lastcharIn == 0x00) // dernier caractere 0x00 donc fin de trame
		{
			if(charIn == 0x00) // debut de trame on traite le message puis on reinitialise 
			{
//				Serial.println(msgbuf);
				int idDev = (msgbuf[0] & 0xF0)>>4;
				if (idDev == RKPID)  // on ne traite que les messages addresses au clavier prinicpal
					{
//          Serial.println(msgbuf);
// test pour fin 
						if (alarm && ((msgbuf[1] & 0x04) == 0) && ((msgbuf[1] & 0x02) == 0))  {
						sendSMS(sendto, "Fin defaut ou alarme" );
						alarm = false;
						}
// test pour fin
						retext = "";
						DecodeAlarm(msgbuf, msgbufLen, retour );  // decodage du message envoye par l alarme
// tests pour voir si l affichage doit etre traite
						for(int n=0;n<(dispBufferlen-1);n++) retext = retext + char(retour[n]);
//            Serial.println(retour);
       // test sur présence A ou W pour envoi du SMS
						if ((retour[17] == 'A') || (retour[18] == 'W')) {
							if(!alarm) {
  							if ((retext != "Not Connected")) {
								sendSMS(sendto, retour);
//								retextprec = ""; 
//								for(int n=0;n<(dispBufferlen-1);n++) retextprec = retextprec + char(retour[n]);
								alarm = true;
								}
							}
						}
/* mis en commentaires pour test fin
						else {
							if (alarm) {
							// messfin.toCharArray(message,30);
							sendSMS(sendto, "Fin defaut ou alarme" );
							alarm = false;
							}
						}
*/
					}
//      for(int n=0;n<30;n++) retourprec[n]=retour[n];   
//				digitalWrite(13,HIGH);
				msgbufLen=0;
				for(int n=0;n<ixMaxPanel;n++) msgbuf[n]=0xFF; // remise à blanc du buffer
				
			}
     lastcharIn = charIn;
     msgbuf[0] = charIn;
		}
		else
		{// on remplit le buffer de caracteres a partir du deuxieme
//		digitalWrite(13,LOW);
		lastcharIn = charIn;
		msgbufLen++;
		msgbuf[msgbufLen] = charIn;
		if (charIn != 0)
			{//wasn't end of packet - is buffer full?
		if (msgbufLen>=ixMaxPanel)
				{//packet never terminated - bytes lost :(
				Serial.println("Buffer overflow");
				}
			}
		}
	 }

// fin loop
}


void sendSMS(char* sendnum, char* messenvoi)
{
// char PIN[5] = "1234";  // code PIN de la carte telephone du module SIM8000
if (!fona.unlockSIM(PIN)) {
          Serial.println(F("Failed"));
//          digitalWrite(4,HIGH);
        } 
        else 
        {
          Serial.println(F("OK!"));
        }
        if (!fona.sendSMS(sendnum, messenvoi)) {
          Serial.print(F("Failed   to send: "));
          Serial.println(messenvoi);
            }
        else {
            Serial.print(F("Sent!: "));
            Serial.println(messenvoi);
            }
         
}
