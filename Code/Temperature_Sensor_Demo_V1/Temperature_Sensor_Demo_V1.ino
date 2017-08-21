/*
  One-Wire Interface
  Components: DS18B20 sensors
  Objective: Reading Temp. from the sensors and Displaying on the screen
  Date: 9 August 2017
*/

#include <OneWire.h> // Library for One-Wire interface
#include <DallasTemperature.h> // Library for DS18B20 temp. sensor

#define ONE_WIRE_GPIO 4 // Data pin of the bus is connected to pin 4

// Setting up the interface for OneWire communication
OneWire oneWire (ONE_WIRE_GPIO);

// Creating an instans of DallasTemperature Class with reference to OneWire interface
DallasTemperature TempSensors(&oneWire);

uint8_t i;

void setup()
{ 
  Serial.begin(9600);
}


void loop() 
{
  
   TempSensors.begin(); // initializing the sensors
   Serial.println("Sensors Initialized"); 
   
  // getting the number of sensors
  uint8_t sensors_count = TempSensors.getDeviceCount();
    // Requesting temp. data 
  Serial.print("Sensors found = ");
  Serial.println(sensors_count);
  
  // Requesting temp. data 
  Serial.println("Requesting Temp. Data"); // Serial indication

  // Recording time in took to respond
  long time_start = millis(); // recording start time
  TempSensors.requestTemperatures(); // requesting data
  long time_end = millis();
  
  // Printing response time
  Serial.print("Done .. ");
  Serial.println(time_end - time_start);

  
  // Printing temperature in a loop
  for (i = 0; i < sensors_count; i++)
  {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" = ");
    Serial.println(TempSensors.getTempCByIndex(i));
    delay(100);
  }

  Serial.println("-------------");
  
  delay(5000);
}
