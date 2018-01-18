#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x3f);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#define INT0_GPIO 2
volatile byte menu = 0;
byte prev_menu = 6;

void setup()
{
  
  //--------------LCD init--------------//
  lcd.begin(16, 2); // initialize the LCD
  lcd.setBacklight(255);

  //--------------Tab Button--------------//
  pinMode(INT0_GPIO, INPUT_PULLUP);
  attachInterrupt(INT0, new_menu, FALLING);
  
} // setup()


//--------------Tab Interrupt--------------//
void new_menu ()
{
  menu ++; 
  // killing some time to reduce switching noise
  for(int i = 0; i < 10000; i++);
} 
//----------------------------------------//


void loop()
{
    if(menu > 3)
      menu = 0;

    if(menu == 0 && prev_menu != 0)
    {
      lcd.clear();
      lcd.print("Main Window");
    }
    if(menu == 1 && prev_menu != 1)
    {
      lcd.clear();
      lcd.print("Tab1");
    }
    if(menu == 2 && prev_menu != 2)
    {
      lcd.clear();
      lcd.print("Tab2");
    }
    if(menu == 3 && prev_menu != 3)
    {
      lcd.clear();
      lcd.print("Tab3");
    }
    prev_menu = menu;

    
    delay(500);
    
/*
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("*** first line.");
    lcd.setCursor(0, 1);
    lcd.print("*** second line.");
*/

} // loop()
