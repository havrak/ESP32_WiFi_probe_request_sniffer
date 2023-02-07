// vim: set ft=arduino:
#include "wifi_sniffer_proxy.h"
#include "EEPROM.h"
#include "Wire.h"
#define EEPROM_SIZE 96

const char* APP_ID = "I4DA";
bool debug =true;
int value = 0;
char buffer [120];
int cnt = 0;
bool connected = 1;

static void emptyEEPROM();
static bool loadFromEEEProm();
static bool storeToEEEPROM();

void emptyEEPROM(){
  for(int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i,0);
}

bool loadFromEEEProm(){
  Serial.println("MAIN | loadFromEEEProm | loading");

	for(int i = 0; i < 4; i++){
		if(EEPROM.read(i)!= APP_ID[i]) return false;
	}

  debug = EEPROM.read(4);
  Serial.print("MAIN | loadFromEEEProm | debug: ");
  Serial.println(debug);
  return true;
}

bool storeToEEEPROM(){
  Serial.println("MAIN | storeToEEEProm | storing");
	for(int i = 0; i < 4; i++){
  EEPROM.write(i,APP_ID[i]);
	}

  EEPROM.write(4,debug);

  EEPROM.commit();
  return true;
}


void processSerialInput() {

  if (Serial.available() == 0) return;
  start:
  while (Serial.available()) {
    char c = Serial.read();
    buffer[cnt++] = c;
    Serial.print(c);
  }
  delay(5);
  if(Serial.available()) goto start;

  buffer[cnt] = '\0';
  cnt = 0;
  if (buffer[0] != 'A' || buffer[1] != 'T'){
    Serial.println("AT+ERR");
    return;
  }
  String tmp(buffer);
  if (tmp.startsWith("AT+DEBG=")) {
     if (isDigit(tmp[8]) == 0) {
       Serial.println("AT+ERR");
       return;
     }
    Serial.println("OK");
    debug = tmp[8]-48 > 0 ? true : false;
  } else if (tmp.startsWith("AT+DEBG?")) {
    Serial.println(debug);
  } else if (tmp.startsWith("AT+LROM")) {
    Serial.println("OK");
    loadFromEEEProm();
  }else if (tmp.startsWith("AT+SROM")) {
    Serial.println("OK");
    storeToEEEPROM();
  } else if (tmp.startsWith("AT+CROM")) {
    Serial.println("OK");
    emptyEEPROM();
  }else{
    Serial.println("AT+ERR");
  }
}

void setup()
{
  EEPROM.begin(EEPROM_SIZE);
  Wire.begin((uint8_t)0x52);
  loadFromEEEProm();
  Serial.print(millis());

  Serial.begin(115200);
  while (!Serial);
	WiFiSnifferProxy::getInstance();

}

void loop() {
  //Serial.println("LOOP");
  WiFiSnifferProxy::getInstance()->switchChannel();
  //uint8_t buffer[1];
  //buffer[0] = WiFiSnifferProxy::getInstance()->checkForSuspiciousDevices();
  //Wire.slaveWrite(buffer, sizeof(buffer));
}
