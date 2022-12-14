#include "DHT.h"
#define DHTPIN D5     // what pin we're connected to
#define DHTTYPE DHT11   // DHT 22  (AM2302)
 
DHT dht(DHTPIN, DHTTYPE);
 
void setup() 
{
  Serial.begin(9600);
  Serial.println("DHTxx test!");
  dht.begin();
}
 
void loop() 
{
  // Wait a few seconds between measurements.
  delay(2000);
 
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = dht.readTemperature(true);
 
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) 
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
 
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t ");
  Serial.print("Temperature: ");
  Serial.print(t - 2);
  Serial.print(" *C ");
  Serial.print(f);
  Serial.println(" *F");
 
}
