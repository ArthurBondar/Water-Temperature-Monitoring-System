/*
   Reading Temperature Data using esp8266 12F
   Date: Nov 21, 2017
*/
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <ESP8266WiFi.h>            // ESP WiFi Libarary
#include <ESP8266HTTPClient.h>      // ESP HTTP client library
#include <PubSubClient.h>           // MQTT publisher/subscriber client 
#include <stdio.h>

#define DEBUG false

//-----------------Dallas_Library-----------------//
#define ONE_WIRE_GPIO 2               // Data pin of the bus is connected to pin 4
#define CALLIBRATION 2                // Used to callibrate the sensors (in C)
volatile uint8_t oneWire_count = 0;
// Setting up the interface for OneWire communication
OneWire oneWire (ONE_WIRE_GPIO);
// Creating an instans of DallasTemperature Class with reference to OneWire interface
DallasTemperature Dallas_Library(&oneWire);
// Sensor structure with address and value
struct oneWire_struct
{
  byte address[8];
  float value;
};

//------------------MQTT-----------------------//
#define MQTT_PORT
#define DEVICE_ID 1
/* Wifi setup */
const char* ssid =        "AER172-2";
const char* password =    "80mawiomi*";
/* MQTT connection setup */
const char* mqtt_user =   "aerlab";
const char* mqtt_pass =   "server";
const char* mqtt_server = "aerlab.ddns.net";
/* Topic Setup */
const char* ID = "2"; // 1 - PVsolar 2 - ThermoDynamics
char gTopic[64];                             // Stores the gTopic being sent
float sampleRate = 1;                        // Sleep time for deepsleep in minutes

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, espClient);
String clientName;

// HTTP
HTTPClient http;

// ------------------SETUP--------------------//
void setup()
{
#if DEBUG // ----------
  Serial.begin(9600);
  Serial.println();
#endif  // ----------

  client.setServer(mqtt_server, 1883);
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(DEVICE_ID);

#if DEBUG // ----------
  Serial.println("Module Name: " + clientName);
  Serial.print("Connecting to ");
  Serial.println(ssid);
#endif // ----------

  // WIFI Settings //
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
    delay(500);

#if DEBUG // ----------
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif  // ----------

}

void loop()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    // check for MQTT Connection
    if (!client.connected())
      reconnect();

    bool   published;             // published succesfully flag
    char   sensor[10];            // sensor gTopic variable
    int    httpCode;              // http return code
    String httpReturn;            // http return payload

    // HTTP Setup //
    http.begin("192.168.2.111", 1880, "/Tank2");
    http.addHeader("Content-Type", "text/plain");

    // Getting Sensor count
    oneWire_count = TempSensors_init();       // Getting number of sensors
    oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors
    // Creating a pointer to pass in the full array of structures
    oneWire_struct *pointer = &TempSensor[0];
    TempSensors_getTemp(&pointer);            // Getting temperature

    /* Publishing number of sensors */
    //MQTT
    sprintf(gTopic, "%s/%s/%s", ID, "Status", "DeviceCount");
    published = client.publish(gTopic, String(oneWire_count).c_str());
    //HTTP
    httpCode = http.POST("DeviceCount:" + String(oneWire_count));
    httpReturn = http.getString();

#if DEBUG // -------------------
    // MQTT
    if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(oneWire_count));
    else          Serial.println("Topic: " + String(gTopic) + " FAILED");
    // HTTP
    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("HTTP return: " + String(httpReturn));
#endif  // ---------------------

    /* Publishing Sensor Data */
    for (uint8_t i = 0; i < oneWire_count; i++)
    {
      sprintf(sensor, "%s%d", "Sensor", i + 1);
      /* Publishing number each temp data */
      // MQTT
      sprintf(gTopic, "%s/%s/%s", ID, "Data" , sensor);
      published = client.publish(gTopic, String(TempSensor[i].value).c_str());
      // HTTP
      httpCode = http.POST("Sensor" + String(i + 1) + ":" + String(TempSensor[i].value));
      httpReturn = http.getString();

#if DEBUG // -------------------
      // MQTT
      if (published) Serial.println("Topic: " + String(gTopic) + " Data: " + String(TempSensor[i].value));
      else           Serial.println("Topic: " + String(gTopic) + " FAILED");
      // HTTP
      Serial.println("HTTP Code: " + String(httpCode));
      Serial.println("HTTP return: " + String(httpReturn));
#endif  // ---------------------
    }/////

    http.end();  //Close connection

  } else {
    WiFi.begin(ssid, password);
  }

  delay(1 * 30000);
}

/*
   If connection failed
*/
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected()) {
#if DEBUG
    Serial.print("Attempting MQTT connection...");
#endif
    // Attempt to connect
    if (client.connect(clientName.c_str(), mqtt_user, mqtt_pass))
    {
#if DEBUG
      Serial.println("connected");
#endif
    }
    else {
#if DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 30 seconds before retrying
      delay(30000);
    }
  }
}

/*
   Internal Mac address to string conversion
*/
String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
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
