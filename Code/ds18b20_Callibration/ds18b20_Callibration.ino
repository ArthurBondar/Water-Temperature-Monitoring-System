/*
  One-Wire Interface
  Components: DS18B20 sensors
  Objective: Finding callibration constants
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
  TempSensors.requestTemperatures();  // requesting data
  // getting the number of sensors
  uint8_t sensors_count = TempSensors.getDeviceCount();
    // Requesting temp. data 
  Serial.print("Sensors found = ");
  Serial.println(sensors_count);

  float sum = 0;
  
  // Printing temperature in a loop
  for (i = 0; i < sensors_count; i++)
  {
    Serial.print("Sensor ");
    Serial.print(i+1);
    Serial.print(" = ");
    Serial.println(TempSensors.getTempCByIndex(i));
    sum += TempSensors.getTempCByIndex(i);
    delay(100);
  }
  Serial.print("avg = ");
  Serial.println(sum/sensors_count);
  Serial.println("-------------");
  delay(500);
}
