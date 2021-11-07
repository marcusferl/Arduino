#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Set the LCD adress to 0x3F for a 20 characters and 4 line display
//LiquidCrystal_I2C lcd(0x3F, 20, 4);
LiquidCrystal_I2C lcd(0x27,20,4);       // Set the I2C address and display size.
 
void setup()  {
  Wire.begin();
//  Serial.begin(115200);
//  while (!Serial);             // Leonardo: wait for serial monitor
//  Serial.println("\nI2C Scanner");

  lcd.init();                    // Columns and Rows or put another way -- width and height
//  lcd.begin(4,5);              // Columns and Rows or put another way -- width and height

  lcd.backlight();
  // lcd.noBacklight();
  lcd.setCursor(0, 0);
  lcd.print("www.LikeCircuit.com");
  lcd.setCursor(1, 1);
  lcd.print("Status: Connected");
//  Serial.println("SETUP");
 }
 
void loop()  {
//   Serial.println("We are here");
   lcd.home(); // set cursor to 0,0 same as lcd.setCursor(0,0);
   delay(3000);
   lcd.setBacklight(LOW);      // Backlight off WORKS
   delay(3000);
   lcd.setBacklight(HIGH);
   lcd.setCursor(0, 0);
   lcd.print("www.LikeCircuit.com");// Backlight on WORKS
   delay(3000);
}
