#include <SD.h>
#include <SPI.h>

// Chip Select pin is tied to pin 8 on the SparkFun SD Card Shield
#define SDCARD_CS_GPIO 4  

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
      if(!header_printed)
      {
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

        dataFile.print(timeStamp);
        dataFile.print(",");
        dataFile.print(100);
        dataFile.print(",");
        dataFile.print(200);
      }
      dataFile.println(); //create a new row to read data more clearly
      dataFile.close();   //close file
   }  
  } 
  delay(500);
}
