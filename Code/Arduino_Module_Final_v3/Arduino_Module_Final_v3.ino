/*
  Arduino Hot-Water-Temp module update with nRF24
  Module ID - 1
  Updated: Oct 8, 2017
*/

#include <Wire.h>                   // I2C Library
#include <LiquidCrystal_PCF8574.h>  // I2C LCD
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <SparkFunDS1307RTC.h>      // RTC Module
#include <SD.h>                     // SD library (used with FAT32 format)
#include <SPI.h>                    // SDcard and nRF interface
#include "RF24.h"                   // nRF24l01 library
#include <printf.h>                 // DEBUG

//-----------------GPIO---------------------//
#define INT0_GPIO 2                 // Menu Button
#define SDCARD_CS_GPIO 4            // Chip Select pin is tied to pin 8 on the SparkFun SD Card Shield 
#define ONE_WIRE_GPIO 5             // Data pin of the bus is connected to pin 4
#define NRF24_GPIO1 6               // nRF CE nRF24(
#define NRF24_GPIO2 7               // nRF CS 
#define INT1_GPIO 3                 // nRF IRQ interrupt pin

//--------------------SD Card---------------------//
bool wrote_in_5min = false;          // flag to write every 5 min
bool new_file = false;               // flag to create new file everyday
bool SD_in = false;

//-----------------Dallas_Library-----------------//
#define CALLIBRATION 2               // Used to callibrate the sensors (in C)
volatile uint8_t oneWire_count = 0;
// Setting up the interface for OneWire communication
OneWire oneWire (ONE_WIRE_GPIO);
// Creating an instans of DallasTemperature Class with reference to OneWire interface
DallasTemperature Dallas_Library(&oneWire);

//--------------------nRF24--------------------------//
RF24 radio(NRF24_GPIO1, NRF24_GPIO2); // nRF object (uint8 CE, uint8 CS)
// Assigning address based on module ID
//*************************************************
#define MODULE_ID 1  // CHANGE TO ASSIGN NEW ID MUST BE UNIQUE    
//*************************************************
const uint64_t address = 0xe7e7e7e700LL | MODULE_ID; // building address
volatile String new_message = " ";
volatile bool nRF_error = true;
// Radio Protocol Codes
#define START_DATA "START"           // Code to start data transmittion
#define RECONFIGURE "CONFIG"         // Code to reconfiger sensor addresses
#define TEMP_TYPE 1

//-----------------LCD Controls------------------//
LiquidCrystal_PCF8574 lcd(0x3f);    // set the LCD address to 0x27 for a 16 chars and 2 line display

// Button
volatile byte menu = 0;
volatile uint8_t _minute = 0;

// Sensor structure with address and value
struct oneWire_struct
{
  byte address[8];
  float value;
};

void setup()
{
  Serial.begin(9600);               // DEBUG
  lcd.begin(16, 2);                 // Dimentions of the LCD
  lcd.setBacklight(255);            // LCD init
  // Menu button interrupt
  pinMode(INT0_GPIO, INPUT_PULLUP); // Pin settup for a button
  attachInterrupt(INT0, new_menu, RISING);  // Pin setup for a button
  // nRF24 new message interrupt
  pinMode(INT1_GPIO, INPUT_PULLUP); // Pin settup for a button
  attachInterrupt(INT1, nRF24_newMessage, FALLING);  // Pin setup for a button
  rtc.begin();                      // RTC init
  pinMode(SDCARD_CS_GPIO, OUTPUT);  // SD card ChipSelect
  nRF24_init();                     // nRF init
}


void loop()   // MAIN LOOP
{
  rtc.update();
  
  oneWire_struct *pointer; 
  // Update Screen every minute
  /*
  if (_minute != rtc.minute())
  {
    _minute = rtc.minute();

*/
  // Getting temperature
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors
  for (int i = 0; i < oneWire_count; i++)
    TempSensors_getSensor(&TempSensor[i]);  // Getting value

  // Writing to SD
  // only writes every 5 miin
  pointer = &TempSensor[0];
  SD_in = SD_write(&pointer);

  //Displayin on the screen
  Display_data(&pointer);
 // }

  if (new_message == START_DATA)
  {
    new_message = " ";
    /*
      According to the protocol, sends:
      1) Module ID + number of sensors
      2) Type + Address + Value
      3) End of tranmittion string EOT
    */
    // HEADER PACKET ----------
    nRF_error = nRF24_send(String(MODULE_ID) + String(oneWire_count));

    // BODY PACKETS -----------
    nRF_error = TempSensors_sendData(&pointer);

    // EOT PACKET --------------
    nRF_error = nRF24_send("EOT");
  }


}
// END MAIN LOOP

/*
   Initializes nRF module
   sets: speed,channel,address etc.
*/
void nRF24_init ()
{
  printf_begin();                     // for DEBUG puposes
  radio.begin();                      // library init
  radio.setPALevel(RF24_PA_MAX);      // Power level (HIGH, LOW, MAX)
  radio.setDataRate(RF24_250KBPS);    // Setting lower data rate
  radio.setRetries(10, 15);           // Retries param1 = 250us*param1 , param2 = count
  radio.setChannel(110);              // Setting COM channel 0-125
  radio.enableDynamicPayloads();      // Calculate message lenght dynamically
  radio.openWritingPipe(address);     // Writing pipe for this module
  radio.openReadingPipe(0, address);  // Reading pipe (should be same as writing)
  radio.maskIRQ(1, 1, 0);             // IRQ pin triggers masking (3 options when to trigger)
  radio.printDetails();               // Printing debug details
  Serial.println("--------------------------------");
  radio.startListening();             // Start the radio listening for data (switch to RX mode)
}

/*
   Reads data from nRF module
   Return: data received - int32
*/
String nRF24_read ()
{
  Serial.println("got message");    // DEBUG
  // Buffers for lenght and data
  uint8_t len = radio.getDynamicPayloadSize();
  char rx_buff[len];                // 32 is the max byte number
  // Reading data
  radio.read(&rx_buff, len);
  Serial.println("Got: *" + String(rx_buff) + "*"); //DEBUG
  String message = String(rx_buff);
  return message;
}

/*
   Sends packet to the server
   0 - server got the packet succesfully
   1 - packet was not received
*/
bool nRF24_send (String _packet)
{
  bool error = 0;                      // Tracks wether packet was received by the server
  char buff[32];                       // Buffer to hold the message
  if (_packet.length() < 32)           // Packets must be under 32 bytes
  {
    // Converting to char array
    _packet.toCharArray(buff, _packet.length() + 1);
    // Sending the data
    radio.stopListening();
    if (!radio.write( &buff, _packet.length()))
      error = 1;
    radio.startListening();
  }
  else
    error = 1;

  return error;
}

/*
   Interrupt 1 subroutine
   Triggered by IRQ pin, changes new message flag
*/
void nRF24_newMessage()
{
  new_message = nRF24_read();
}

/*
   Initialized oneWire sensors + gets the number of sensors
*/
uint8_t TempSensors_init ()
{
  uint8_t _count;
  Dallas_Library.begin();               // initializing the sensors
  _count = Dallas_Library.getDeviceCount();// getting number of sensors
  Dallas_Library.requestTemperatures(); // requesting data
  return _count;
}

/*
   Reading one sensor
   Modifing pointer for one sensor with address and value
*/
void TempSensors_getSensor( oneWire_struct *_sensor)
{
  oneWire.search(_sensor->address);
  _sensor->value = Dallas_Library.getTempC(_sensor->address);
  _sensor->value -= CALLIBRATION;
}

/*
   Sends packets of temperature data:
   Type[1byte], Address[2-8bytes], Data[4bytes]
   0 - server received all the packets
   1 - one or more packets got lost
*/
bool TempSensors_sendData ( oneWire_struct **_sensor )
{
  bool error = 0;
  String sAddress;

  for ( int i = 0; i < oneWire_count; i++)
  {
    // Converting byte array to string
    for (int j = 0; j < 8; j++)
      sAddress += String((*_sensor+i)->address[j]);
    // Sending as strings
    Serial.println(sAddress + "  " + String((*_sensor+i)->value)); // DEBUG
    error = nRF24_send(String(MODULE_ID) + String(TEMP_TYPE) + sAddress + String((*_sensor+i)->value));
  }
  return error;
}

/*
   Interrupt sub-routine
   increments the menu variable to change screen
   Soldered 220nano debounce cap
*/
void new_menu ()
{
  menu ++;                            //inc. tab counter
  // tab counter reached limit
  if (menu > (oneWire_count / 4) + 1)
    menu = 0;
}

/*
   Function that write temp values to SD card
   Writes every 5 min, only if SDcard present
*/
bool SD_write ( oneWire_struct **_sensor )
{
  SDClass SD;                         // Create instance of SDclass
  bool sd_in = false;                 // SD in flag
  if (SD.begin(SDCARD_CS_GPIO))       // Writing to SD Card only when its present
  {
    sd_in = true;
    // Printing data every 5 min
    if ( rtc.minute() % 5 == 0)
    {
      if (!wrote_in_5min)
      {
        wrote_in_5min = true;
        // Creating new file for every day, title - date
        const String FileName = String(rtc.date()) + "_" + String(rtc.month()) + "_" + String(rtc.year()) + ".csv";
        bool header_printed = SD.exists(FileName);// Check if file already exist
        File dataFile = SD.open(FileName, FILE_WRITE);// File Pointer
        // Writing to the file only if its open
        if (dataFile)
        {
          // HEADER //
          if (!header_printed)                    // printing headers
          {
            dataFile.print("Date,Time");

            for (int i = 0; i < oneWire_count; i++) // Printing sensor headers
              dataFile.print(",Sensor " + String(i + 1));

            dataFile.println();                  //create a new row to read data more clearly
          }
          // VALUES //
          // Date
          dataFile.print(String(rtc.year()) + "/" + String(rtc.month()) + "/" + String(rtc.date()) + ",");
          // Time
          String Time = String(rtc.hour()) + ":";
          if (rtc.minute() < 10)
            Time = Time + "0" + String(rtc.minute());
          else
            Time = Time + String(rtc.minute());
          dataFile.print(Time);                 // Printing Time Stamp
          // Values
          for (int i = 0; i < oneWire_count; i++) // Printing sensor headers
          {
            dataFile.print(",");
            dataFile.print((*_sensor + i)->value);
          }
          dataFile.println();                     //create a new row to read data more clearly
          dataFile.close();                       //close file
        }
      }
    }
    else
      wrote_in_5min = false;
  }
  return sd_in;
}

/*
   Displays data fron sensors
   prints the header page + following sensor pages
*/
void Display_data ( oneWire_struct **_sensor )
{
  if (menu > (oneWire_count / 4) + 1)         // Overflow the button
    menu = 0;

  if (menu == 0 || oneWire_count == 0)        // Main menu
  {
    lcd.clear();
    String Date = String(rtc.year()) + "/" + String(rtc.month()) + "/" + String(rtc.date());
    String Time = String(rtc.hour()) + ":";
    if (rtc.minute() < 10)
      Time = Time + "0" + String(rtc.minute());
    else
      Time = Time + String(rtc.minute());
    lcd.print(Date + " " + Time);
    if (SD_in) lcd.print(" SD");
    lcd.setCursor(0, 1);
    lcd.print("Found:" + String(oneWire_count));
    lcd.print(" Link:");
    if (nRF_error)
      lcd.print("NO");
    else
      lcd.print("OK");
  }
  else // in one of the tabs
  {
    lcd.clear();
    for (int i = 0; i < 4; i++)
    {
      if ( ((menu - 1) * 4 + i) <= oneWire_count - 1 ) // Tab menu functions
      {
        lcd.setCursor((i * 8) % 16, i / 2);   // Positioning algorithm
        lcd.print(String((menu - 1)*4 + i + 1) + ":");
        lcd.print((*_sensor + ((menu - 1)*4 + i))->value, 1);
      }
    }
  }

}


