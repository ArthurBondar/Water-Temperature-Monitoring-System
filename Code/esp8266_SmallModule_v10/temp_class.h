/*
    This class is used to read Temp data
    from Dallas OneWire bus interface
*/


#ifndef Temp_Class_h
#define Temp_Class_h

#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor

struct oneWire_struct
{
  byte address[8];
  float value;
};

class Temp_Class
{
  private:
    int _gpio = 0;
    uint8_t _count = 0;
    OneWire* _oneWire;
    DallasTemperature* _dallaslib;

  public:
    // Constructor
    Temp_Class(int gpio)
    {
      if (gpio > 0 && gpio < 17) _gpio = gpio;
      else return;

      _oneWire = new OneWire(_gpio);
      // Creating an instans of DallasTemperature Class with reference to OneWire interface
      _dallaslib = new DallasTemperature(_oneWire);
    };
    // Destructor
    ~Temp_Class()
    {
      delete _oneWire;
      delete _dallaslib;
    }
    // Initialized oneWire sensors + gets the number of sensors
    uint8_t init()
    {
      _dallaslib->begin();               // initializing the sensors
      _count = _dallaslib->getDeviceCount();// getting number of sensors
      _dallaslib->requestTemperatures(); // requesting data
      return _count;
    }
    // Modifing pointer for one sensor with address and value
    void getTemp( oneWire_struct **_sensor)
    {
      for (int i = 0; i < _count; i++)
      {
        _oneWire->search((*_sensor + i)->address);
        (*_sensor + i)->value = _dallaslib->getTempC((*_sensor + i)->address);
      }
    }
};

#endif
