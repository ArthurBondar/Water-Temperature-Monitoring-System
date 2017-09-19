#include <Wire.h>                   // I2C Library
#include <LiquidCrystal_PCF8574.h>  // I2C LCD
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <SparkFunDS1307RTC.h>      // RTC Module
#include <SD.h>                     // SD library (used with FAT32 format)
#include <SPI.h>


//-----------------SD Card---------------------//
#define SDCARD_CS_GPIO 3              // Chip Select pin is tied to pin 8 on the SparkFun SD Card Shield 
bool wrote_in_5min = false;           // flag to write every 5 min
bool new_file = false;                // flag to create new file everyday

//-----------------TempSensors---------------------//
#define ONE_WIRE_GPIO 5              // Data pin of the bus is connected to pin 4
const uint8_t CALLIBRATION = 1;      // Used to callibrate the sensors (in C)
volatile uint8_t sensors_count = 0;
// Setting up the interface for OneWire communication
OneWire oneWire (ONE_WIRE_GPIO);
// Creating an instans of DallasTemperature Class with reference to OneWire interface
DallasTemperature TempSensors(&oneWire);

//-----------------LCD Controls---------------------//
LiquidCrystal_PCF8574 lcd(0x3f);    // set the LCD address to 0x27 for a 16 chars and 2 line display
// Tab Button
#define INT0_GPIO 2
volatile byte menu = 0;

void setup() 
{
   // LCD init //
  lcd.begin(16, 2);                 // Dimentions of the LCD
  lcd.setBacklight(255);
  // Tab Button //
  pinMode(INT0_GPIO, INPUT_PULLUP);
  attachInterrupt(INT0, new_menu, RISING);
  // RTC //
  rtc.begin();                      // initialize the library
  // SDcard //
  pinMode(SDCARD_CS_GPIO, OUTPUT);
  // Display
  menu = 0;
}

void loop() 
{
  //-----------------TempSensors---------------------//
  TempSensors.begin();                // initializing the sensors
  sensors_count = TempSensors.getDeviceCount();
  TempSensors.requestTemperatures();  // requesting data
  float temp[sensors_count];          // storing the data
  float avg_temp = 0;                 // storing average temp
  // storing data from sensors + calc avg temp
  if(sensors_count)                   // executes when sensors present         
  {
    for (int i = 0; i < sensors_count; i++)
    { 
      temp[i] = TempSensors.getTempCByIndex(i);
      temp[i] -= CALLIBRATION;        // Callibration constant
      avg_temp += temp[i];            // saving the sum
    }
    avg_temp /= sensors_count; 
  }
  // End TempSensors

  //-----------------RTC---------------------//
  rtc.update();                       // updates: rtc.seconds(), rtc.minutes() etc..
  String Date = String(rtc.date()) + "/" + String(rtc.month()) + "/" + String(rtc.year());
  String Time = String(rtc.hour()) + ":";
  if(rtc.minute() < 10)
    Time = Time + "0" + String(rtc.minute());
  else
    Time = Time + String(rtc.minute());
  // End RTC

  //-----------------SDcard---------------------//
  SDClass SD;                         // Create instance of SDclass
  bool sd_in = false;                 // SD in flag
  if (SD.begin(SDCARD_CS_GPIO))       // Writing to SD Card only when its present
  {
    sd_in = true;                   
    // Printing data every 5 min
    if( rtc.minute() % 5 == 0)
    {
       if(!wrote_in_5min)
       {
          wrote_in_5min = true;
          const String FileName = String(rtc.date()) + "_" + String(rtc.month()) + "_" + String(rtc.year()) + ".csv";
          bool header_printed = SD.exists(FileName);// Check if file already exist
          File dataFile = SD.open(FileName, FILE_WRITE);
          
          if (dataFile)   
          {  
            if(!header_printed)                     // printing headers
            {
              dataFile.print("Date");               // Printing time header 
              dataFile.print(",");    
              dataFile.print("Time");               // Printing time header 
              dataFile.print(",");
              dataFile.print("Average");            // Printing avg header 
              for(int i = 0; i < sensors_count; i++)// Printing sensor headers
              {   
                dataFile.print(",");
                dataFile.print("Sensor " + String(i+1)); 
              } 
              dataFile.println();                  //create a new row to read data more clearly
            }
    
            dataFile.print(String(rtc.year()) + "/" + String(rtc.month()) + "/" + String(rtc.date()));                 // Printing time
            dataFile.print(",");    
            dataFile.print(Time);                 // Printing time header 
            dataFile.print(",");
            dataFile.print(avg_temp);             // Printing time
            for(int i = 0; i < sensors_count; i++)// Printing sensor headers
            {   
              dataFile.print(",");
              dataFile.print(temp[i]); 
            }
            dataFile.println();                     //create a new row to read data more clearly
            dataFile.close();                       //close file
        } 
      }
    }
    else
      wrote_in_5min = false;  
  } 
  // End SDcard

  //-----------------Display---------------------//
  
  if(menu > (sensors_count/4) + 1)            // Overflow the button
     menu = 0;

  if(menu == 0 || sensors_count == 0)         // Main menu
  {
    lcd.clear();
    lcd.print(Date + " " + Time);
    if(sd_in) lcd.print(" SD");
    lcd.setCursor(0, 1);
    lcd.print("All:" + String(sensors_count));
    lcd.print(" Avg:" + String(avg_temp));
  }
  else // in one of the tabs
  {
    lcd.clear();
    for (int i = 0; i<4; i++)
    {
       if ( (menu-1)*4 + i <= sensors_count-1 ) // Tab menu functions
       {  
          lcd.setCursor((i*8)%16, i/2);         // Positioning algorithm
          lcd.print(String((menu-1)*4 + i + 1) + ":"); 
          lcd.print(temp[(menu-1)*4 + i], 1);
       }
    }
  }
  // End Display
  
}  // End Main

//--------------Tab Interrupt--------------//
void new_menu ()
{
  menu ++;                            //inc. tab counter
  // tab counter reached limit
  if(menu > (sensors_count/4) + 1)
   menu = 0;
  // killing some time to reduce switching noise
  delay(5);
} //--------------------------------------//
