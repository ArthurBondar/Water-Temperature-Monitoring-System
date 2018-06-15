# Water Temperature Monitoring System
### One-Wire Sensors Monitoring and Data Logging system

Components:
* Arduino Nano
* SparkFun RTC (company code BOB-12708) model DS1307
* 16x4 LCD + PCF8574A I2C gpio adapter/extension 
* SDHC card reader

Principle of operation:
* Scanning for One-Wire sensors
* Getting temperature and time from RTC
* Displaying on the LCD
* Writing to SDcard
