/*
    DS18B20 Basic Code
    Temperatur auslesen mit einem DS18B20 Temperaturfühlers
    Created by cooper, 2020
    my.makesmart.net/user/cooper
*/

#include <OneWire.h>
#include <DallasTemperature.h>

// Der PIN D2 (GPIO 4) wird als BUS-Pin verwendet
#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

// In dieser Variable wird die Temperatur gespeichert
float temperature;


void setup(){
  Serial.begin(115200);

  // DS18B20 initialisieren
  DS18B20.begin();
}

void loop(){
  
  DS18B20.requestTemperatures();
  temperature = DS18B20.getTempCByIndex(0);

  // Ausgabe im seriellen Monitor
  Serial.println(String(temperature) + " °C");

  // 5 Sekunden warten
  delay(5000);

}
