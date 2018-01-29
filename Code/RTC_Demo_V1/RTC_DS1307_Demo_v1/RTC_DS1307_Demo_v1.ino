#include <Wire.h>
#include <SparkFunDS1307RTC.h>


void setup() 
{
  Serial.begin(9600);

  rtc.begin(); // initialize the library
}

void loop() 
{
  static int8_t lastSecond = -1;

  rtc.update(); // updates: rtc.seconds(), rtc.minutes() etc..

  if(rtc.second() != lastSecond)
  {
    Serial.println(printDate());
    Serial.println(printTime());
    Serial.println("------------");
    lastSecond = rtc.second();
  }

}

String printDate()
{
  String Date;
  Date = String(rtc.date()) + "/" + String(rtc.month()) + "/" + String(rtc.year());
  return Date;
}

String printTime()
{
  String Time;
  Time = String(rtc.hour()) + ":" + String(rtc.minute()) + ":" + String(rtc.second());
  return Time;
}
