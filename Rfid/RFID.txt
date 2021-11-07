#include <ESP8266WiFi.h>
#include "MFRC522.h"



#define RST_PIN 0 // RST-PIN for RC522 - RFID - SPI - Module GPIO-0 
#define SS_PIN  15  // SDA-PIN for RC522 - RFID - SPI - Module GPIO-15
#define RELAY_PIN 5 // RELAY-PIN in GPI0-16

MFRC522 mfrc522(SS_PIN, RST_PIN);  



void setup() {

  pinMode(RELAY_PIN, OUTPUT);
  
  Serial.begin(115200);
  delay(10);
  SPI.begin();           
  mfrc522.PCD_Init();    
  
}

void loop() { 
  String rfid = "";
  
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  }
  
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    delay(50);
    return;
  }
for (int i = 0; i < mfrc522.uid.size; i++) 
{
Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
Serial.print(mfrc522.uid.uidByte[i], HEX);
rfid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
rfid.concat(String(mfrc522.uid.uidByte[i], HEX));
}
rfid.toUpperCase();

if(rfid.substring(1) == "5A E7 62 1A" || rfid.substring(1) == "27 2E 12 B3" )
 {
 digitalWrite(RELAY_PIN, HIGH); //Relay ON
 delay(1500);
 digitalWrite(RELAY_PIN, LOW); //Relay OFF
 }
 delay(5000);
}
       