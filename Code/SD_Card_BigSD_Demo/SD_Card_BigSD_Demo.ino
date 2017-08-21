/* 
 *  Testing with SDHC 1 big SD card
 *  note: works only with 5v supplied to +5v pin (not 3.3)
 */

#include <SD.h>
#include <SPI.h>

// Chip Select pin is tied to pin 8 on the SparkFun SD Card Shield
#define SDCARD_CS_GPIO 8  

bool header_printed = false;

const String FileName = "datalog.csv";

void setup()
{
  Serial.begin(9600);
  
  pinMode(SDCARD_CS_GPIO, OUTPUT);
}

void loop()
{
  SDClass SD;
  
  // Writing to SD Card only when its present
  if (SD.begin(SDCARD_CS_GPIO)) 
  {
    
    File dataFile = SD.open(FileName, FILE_WRITE);
  
    if (dataFile)   
    {  
      Serial.println("Datafile Created");
      if(!header_printed)
      {
        Serial.println("Header Printed");  
        header_printed = true;
        dataFile.print("Time");
        dataFile.print(",");
        dataFile.print("Sensor1");
        dataFile.print(",");
        dataFile.print("Sensor2"); 
      }
      else
      {
        int timeStamp = millis();
        Serial.println("Data Printed");
        dataFile.print(timeStamp);
        dataFile.print(",");
        dataFile.print(100);
        dataFile.print(",");
        dataFile.print(200);
      }
      dataFile.println(); //create a new row to read data more clearly
      dataFile.close();   //close file
   }
   else  
   {
     Serial.println("Cannot open File");
   }
  }
  else  
  {
    Serial.println("SD card not present");
  } 
  delay(500);
}
