/*
  Arduino Hot-Water-Temp module update with nRF24
  Module ID - 1
  Updated: Oct 14, 2017
*/

#include <Wire.h>                   // I2C Library
#include <LiquidCrystal_PCF8574.h>  // I2C LCD
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <SparkFunDS1307RTC.h>      // RTC Module
#include <SD.h>                     // SD library (used with FAT32 format)
#include <SPI.h>                    // SDcard and nRF interface
#include "RF24.h"                   // nRF24l01 library

//-----------------GPIO---------------------//
#define INT0_GPIO 2                 // Menu Button
#define SDCARD_CS_GPIO 4            // Chip Select pin is tied to pin 8 on the SparkFun SD Card Shield 
#define ONE_WIRE_GPIO 5             // Data pin of the bus is connected to pin 4
#define NRF24_GPIO1 6               // nRF CE nRF24(
#define NRF24_GPIO2 7               // nRF CS 
#define INT1_GPIO 3                 // nRF IRQ interrupt pin

//--------------------SD Card---------------------//
bool wrote_in_5min = false;          // flag to write every 5 min
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
#define ADDRESS 0xe7e7e7e700LL | MODULE_ID // building address
volatile bool nRF_error = false;
volatile bool db_connection = false;
volatile bool connection_checked = false;
// Radio Protocol Codes
#define START_DATA "START"           // Symbol to start data transmittion
#define DELIM ' '                    // Delimiter betweeen values
#define END_DATA "EOT"               // Symbol to end trasmition

//-----------------LCD Controls------------------//
LiquidCrystal_PCF8574 lcd(0x3f);    // set the LCD address to 0x27 for a 16 chars and 2 line display

// Button
volatile byte menu = 0;

// Sensor structure with address and value
struct oneWire_struct
{
  byte address[8];
  float value;
};

void setup()
{
  lcd.begin(16, 2);                 // Dimentions of the LCD
  lcd.setBacklight(255);            // LCD init
  // Menu button interrupt
  pinMode(INT0_GPIO, INPUT_PULLUP); // Pin settup for a button
  attachInterrupt(INT0, new_menu, FALLING);  // Pin setup for a button
  rtc.begin();                      // RTC init
  pinMode(SDCARD_CS_GPIO, OUTPUT);  // SD card ChipSelect
  nRF24_init();                     // nRF init
}


void loop()   // MAIN LOOP
{
  rtc.update();                             // Getting new time reading
  /* Data Base connection flag */
  if (rtc.minute() % 15 == 0)               // reset the flag evey 15 min
  {
    if(!connection_checked)
    {
      connection_checked = true;
      db_connection = false;
    }
  }
  else
    connection_checked = false;

  // Getting Sensor count
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors

  // Creating a pointer to pass in the full array of structures
  oneWire_struct *pointer = &TempSensor[0];

  // Getting temperature
  TempSensors_getTemp(&pointer);

  // Writing to SD
  SD_in = SD_write(&pointer);

  //Displayin on the screen
  Display_data(&pointer);

  // If Server sent message
  if (radio.available())
  {
    char buff[32];                         // Buffer to read nRF24 data

    nRF24_read(buff);
    if (strcmp(buff, START_DATA) == 0) // starting the tranmittion
    {
      nRF_error = false;                   // Reseting error flag
      /*
        According to the protocol, sends:
        1) Module ID + number of sensors
        2) Type + Address + Value
        3) End of tranmittion string EOT
      */
      // HEADER PACKET
      sprintf(buff, "%i%c%i", MODULE_ID, DELIM, oneWire_count);
      nRF_error |= nRF24_send(buff, strlen(buff));
      delay(10);

      // BODY PACKETS
      nRF_error |= nRF24_sendTemp(&pointer);
      delay(10);

      // EOT PACKET
      sprintf(buff, "%s", END_DATA);
      nRF_error |= nRF24_send(buff, strlen(buff));

      if (!nRF_error) db_connection = true; // Data sent to DB succesfully
    }
  }

}
// END MAIN LOOP

/*
   Initializes nRF module
   sets: speed,channel,address etc.
*/
void nRF24_init ()
{
  radio.begin();                      // library init
  radio.setPALevel(RF24_PA_MAX);      // Power level (HIGH, LOW, MAX)
  radio.setDataRate(RF24_250KBPS);    // Setting lower data rate
  radio.setRetries(10, 15);           // Retries param1 = 250us*param1 , param2 = count
  /* Setting COM channel, increment the channel every 5 receivers */
  radio.setChannel(100 + (int)(MODULE_ID / 5)); // Setting COM channel 0-125
  //radio.setChannel(110);// Setting COM channel 0-125
  radio.enableDynamicPayloads();      // Calculate message lenght dynamically
  radio.openWritingPipe(ADDRESS);     // Writing pipe for this module
  radio.openReadingPipe(0, ADDRESS);  // Reading pipe (should be same as writing)
  radio.startListening();             // Start the radio listening for data (switch to RX mode)
}

/*
   Reads data from nRF module
   Return: data received as string
*/
void nRF24_read (char *rx_buff)
{
  // Buffers for lenght and data
  uint8_t len = radio.getDynamicPayloadSize();
  // Reading data
  radio.read(rx_buff, len);
}

/*
   Sends packet to the server
   0 - server got the packet succesfully
   1 - packet was not received
*/
bool nRF24_send (char *_packet, uint8_t lenght)
{
  bool error = 0;                      // Tracks wether packet was received by the server

  if (lenght > 32)            // Packets must be under 32 bytes
    error = 1;
  else
  {
    // Sending the data
    radio.stopListening();
    if (!radio.write( _packet, lenght + 1))
      error = 1;
    radio.startListening();
  }

  return error;
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
void TempSensors_getTemp( oneWire_struct **_sensor)
{
  for (int i = 0; i < oneWire_count; i++)
  {
    oneWire.search((*_sensor + i)->address);
    (*_sensor + i)->value = Dallas_Library.getTempC((*_sensor + i)->address);
    (*_sensor + i)->value -= CALLIBRATION;
  }
}

/*
   Sends packets of temperature data:
   Type[1byte], Address[2-8bytes], Data[4bytes]
   0 - server received all the packets
   1 - one or more packets got lost
*/
bool nRF24_sendTemp ( oneWire_struct **_sensor )
{
  char cAddress[16]; char _message[32]; char _value[5];
  bool error = 0;
  for ( int i = 0; i < oneWire_count; i++)
  {
    /* Converting byte array to string */
    to_string( cAddress, (*_sensor + i)->address);
    /* Converting float to string */
    dtostrf((*_sensor + i)->value, 2, 2, _value);
    // Building the string
    sprintf(_message, "%s%c%s", cAddress, DELIM, _value);
    error |= nRF24_send(_message, strlen(_message));
    delay(10);
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
  char FileName[13]; char _date[9]; char _time[6]; char sAddress[17];
  bool _sd_in = false;                 // SD in flag
  SDClass SD;                          // Create instance of SDclass

  /* If SD Card is present */
  if (SD.begin(SDCARD_CS_GPIO))
  {
    _sd_in = true;
    /* 5 min past since the last print */
    if (rtc.minute() % 5 == 0)
    {
      /* Havent printed in this minute window */
      if (!wrote_in_5min)
      {
        wrote_in_5min = true;
        // Creating new file for every day, title - date
        get_Date(_date, '_' , 0);
        strcpy(FileName, _date);
        strcat(FileName, ".csv");
        // Flag for header is on when file exist
        bool header_printed = SD.exists(FileName);     // Check if file already exist
        File dataFile = SD.open(FileName, FILE_WRITE); // File Pointer
        /* Writing to the file only if its open */
        if (dataFile)
        {
          // -- HEADERS -- //
          if (!header_printed)                         // printing headers
          {
            /* First Row - Date, Time, Sensor[i] */
            dataFile.print("Date,Time");
            for (int i = 0; i < oneWire_count; i++)    // Printing sensor index
            {
              dataFile.print(",Sensor ");
              dataFile.print(i);
            }
            dataFile.println();                        // END ROW
            /* Second Row - Address[i] */
            dataFile.print(",Address");
            for (int i = 0; i < oneWire_count; i++)    // Printing sensor addresses
            {
              dataFile.print(",");
              /* Converting byte array to string */
              to_string(sAddress, (*_sensor + i)->address);
              dataFile.print(sAddress);
            }
            dataFile.println();                        //END ROW
          }
          // -- VALUES -- //
          // Date
          get_Date(_date, '/', 1);
          dataFile.print(_date);
          dataFile.print(",");
          // Time
          get_Time(_time, ':');
          dataFile.print(_time);                      // Printing Time Stamp
          // Values
          for (int i = 0; i < oneWire_count; i++)     // Printing sensor values
          {
            dataFile.print(",");
            dataFile.print((*_sensor + i)->value);
          }
          dataFile.println();                         //create a new row to read data more clearly
          dataFile.close();                           //close file
        }
      }
    }
    else  /* Reseting the flag when passed the 5 min window */
      wrote_in_5min = false;                      // Reseting the printing flag
  }
  return _sd_in;
}

/*
   This function returns string with the current date
   @inparam delimiter
   output: day+delim+month+delim+year+\0 (9 characters)
   if for SD card - year and month switch
*/
void get_Date ( char *str, char delim, bool for_SD )
{
  if (for_SD)
  {
    if (rtc.month() < 10 && rtc.date() < 10)
      sprintf(str, "%i%c0%i%c0%i", rtc.year(), delim, rtc.month(), delim, rtc.date());
    else if (rtc.date() < 10)
      sprintf(str, "%i%c%i%c0%i", rtc.year(), delim, rtc.month(), delim, rtc.date());
    else if (rtc.month() < 10)
      sprintf(str, "%i%c0%i%c%i", rtc.year(), delim, rtc.month(), delim, rtc.date());
    else
      sprintf(str, "%i%c%i%c%i", rtc.year(), delim, rtc.month(), delim, rtc.date());
  }
  else
  {
    if (rtc.month() < 10 && rtc.date() < 10)
      sprintf(str, "0%i%c0%i%c%i", rtc.date(), delim, rtc.month(), delim, rtc.year());
    else if (rtc.date() < 10)
      sprintf(str, "0%i%c%i%c%i", rtc.date(), delim, rtc.month(), delim, rtc.year());
    else if (rtc.month() < 10)
      sprintf(str, "%i%c0%i%c%i", rtc.date(), delim, rtc.month(), delim, rtc.year());
    else
      sprintf(str, "%i%c%i%c%i", rtc.date(), delim, rtc.month(), delim, rtc.year());
  }
}

/*
  This function returns string with the current time
  @inparam delimiter
  output: hour+delim+minute+\0 (6 characters)
*/
void get_Time ( char *str, char delim )
{
  if (rtc.hour() < 10 && rtc.minute() < 10)
    sprintf(str, "0%i%c0%i", rtc.hour(), delim, rtc.minute());
  else if (rtc.hour() < 10)
    sprintf(str, "0%i%c%i", rtc.hour(), delim, rtc.minute());
  else if (rtc.minute() < 10)
    sprintf(str, "%i%c0%i", rtc.hour(), delim, rtc.minute());
  else
    sprintf(str, "%i%c%i", rtc.hour(), delim, rtc.minute());
}

/*
   Converts 8 byte array -> 16 byte string + 1 byte '/0'
*/
void to_string (char *str, byte *_source )
{
  sprintf(str, "%X%X%X%X%X%X%X%X", *_source, *(_source + 1), *(_source + 2), *(_source + 3), *(_source + 4), *(_source + 5), *(_source + 6), *(_source + 7));
}

/*
   Displays data fron sensors
   prints the header page + following sensor pages
*/
void Display_data ( oneWire_struct **_sensor )
{
  char _date[9]; char _time[6];

  if (menu > (oneWire_count / 4) + 1)         // Overflow the button
    menu = 0;

  lcd.setCursor(0, 0);
  /* --- MAIN WINDOW ---- */
  if (menu == 0 || oneWire_count == 0)
  {
    lcd.setCursor(0, 0);
    // Printing Date and Time
    get_Date (_date, '/', 0);
    get_Time (_time, ':');
    lcd.print(_date);
    lcd.print(" ");
    lcd.print(_time);
    // SD card indicator
    if (SD_in)
      lcd.print(" S");
    else
      lcd.print("  ");
    // Number of sensor connected
    lcd.setCursor(0, 1);
    lcd.print("Found:");
    if(oneWire_count == 0)
      lcd.print("0 ");          // clearing the screen
    else if(oneWire_count > 9)
      lcd.print(oneWire_count); // printing two digit
    else
    {
      lcd.print(oneWire_count); // printing one digit
      lcd.print(" ");
    }
    // Link to the data base
    lcd.print(" Link:");
    if (db_connection)
      lcd.print("OK");
    else
      lcd.print("NO");
  }
  else
  { /* --- SENSOR WINDOWS --- */
    for (uint8_t i = 0; i < 4; i++)
    {
      if ( ((menu - 1) * 4 + i) <= oneWire_count - 1 ) // Tab menu functions
      {
        lcd.setCursor((i * 8) % 16, i / 2);   // Positioning algorithm
        lcd.print((menu - 1) * 4 + i + 1);
        lcd.print(":");
        lcd.print((*_sensor + ((menu - 1) * 4 + i))->value, 1);
        if ( ((menu - 1) * 4 + i + 1) > 9)
          lcd.print(" ");
        else
          lcd.print("  ");
      }
    }
    /* Clearing the rest of the screen */
    if ( oneWire_count % 4 == 1 && menu == ((oneWire_count-1)/4+1) )
    {
      lcd.print("        "); // x8
      lcd.setCursor(0, 1);
      lcd.print("                "); // x16
    }
    if (oneWire_count % 4 == 2 && menu == ((oneWire_count-1)/4+1))
    {
      lcd.setCursor(0, 1);
      lcd.print("                "); // x16
    }
    if (oneWire_count % 4 == 3 && menu == ((oneWire_count-1)/4+1))
      lcd.print("        "); // x8
  }

}


