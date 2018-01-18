#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
 
void setup() 
{
  Serial.begin(9600); 
  
  // Setting up the display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.display();
  delay(500);
  
  display.setTextColor(WHITE);
  display.setTextSize(1);
}

void loop() 
{
  display.clearDisplay();

  // Time
  display.setCursor(0, 0);
  display.println("Time:" + String(millis()));
  
  // Sensor 1
  display.print("S1:");
  display.print("25.3");
  // Sensor 2
  display.setCursor(66, 8);
  display.print("S2:");
  display.println("28.1");

  // Sensor 3
  display.print("S3:");
  display.print("27.9");
  // Sensor 4
  display.setCursor(66, 16);
  display.print("S4:");
  display.println("22.1");
  
  // Sensor 5
  display.print("S5:");
  display.print("23.3");
  // Sensor 6
  display.setCursor(66, 24);
  display.print("S6:");
  display.println("28.1");

  // Displaying and waiting
  
  display.display();
  delay(100);
}
