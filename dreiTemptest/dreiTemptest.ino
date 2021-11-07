#include "DHT.h"
#define DHTPIN11 D6
#define DHTPIN22 D7
#define DHTTYPE11 DHT11
#define DHTTYPE22 DHT22
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 14 //GPO 14

#include <LiquidCrystal_I2C.h> //LCD


DHT dht1(DHTPIN11, DHTTYPE11);
DHT dht2(DHTPIN22, DHTTYPE22);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
LiquidCrystal_I2C lcd(0x27,20,4); //SCL -> D1 /SDA -> D2

void setup() {

  //LCD Setup
 lcd.init();
 lcd.backlight();
 
 Serial.begin(115200);
 dht1.begin();
 dht2.begin();
 DS18B20.begin();
}

void loop() {
DS18B20.requestTemperatures();
float DHT11_t = dht1.readTemperature();
float DHT22_t = dht2.readTemperature();
float DALLAS_t = DS18B20.getTempCByIndex(0);

lcd.setCursor(0, 0);
lcd.print("DHT 11 Sensor");
Serial.print("\n");
Serial.print("DHT11 ");
Serial.print("\n");
lcd.setCursor(0, 1);
lcd.print(String(DHT11_t) +" C ");
Serial.print(DHT11_t);Serial.print(String(char(176))+"C ");
Serial.print("\n");
delay(4000);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("DHT 22 Sensor");
Serial.print("DHT22 ");
Serial.print("\n");
lcd.setCursor(0, 1);
lcd.print(String(DHT22_t) + " C ");
Serial.print(DHT22_t);Serial.print(String(char(176))+"C ");
Serial.print("\n");
delay(4000);
lcd.clear();
lcd.setCursor(0, 0);
lcd.print("Dallas Sensor");
Serial.print("Dallas ");
Serial.print("\n");
lcd.setCursor(0, 1);
lcd.print(String(DALLAS_t) + " C ");
Serial.print(String(DALLAS_t) + "C ");
delay(4000);
lcd.clear();
}
