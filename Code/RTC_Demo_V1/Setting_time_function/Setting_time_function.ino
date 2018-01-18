/*  Sets time on the RTC_DS1307
 */

#include <Wire.h>
#include <SparkFunDS1307RTC.h>


void setup() 
{
  Serial.begin(9600);

  rtc.begin(); // initialize the library

  //rtc.autoTime(); // sets the time automatically few sec lagg
  rtc.setTime(30, 36, 12, 3, 19, 9, 17); // manually set time
  //          sec,min,hr,day,date,month,year
  // rtc.set12Hour(); // 12-hour mode

  Serial.println(printDate());
  Serial.println(printTime());

}

void loop ()
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

