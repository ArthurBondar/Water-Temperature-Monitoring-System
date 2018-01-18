/*
   Reading Temperature Data using esp8266 12F
   Date: Oct 31, 2017
*/
#include <OneWire.h>                // Library for One-Wire interface
#include <DallasTemperature.h>      // Library for DS18B20 temp. sensor
#include <ESP8266WiFi.h>            // ESP WiFi Libarary
#include <PubSubClient.h>           // MQTT publisher/subscriber client 

#define DEBUG true

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
const char* location =        "AERlab";
const char* systemName =      "WaterTanks";
const char* subSystem =       "Tank1";
const char* transducerType =  "Temperature";
char topic[64];                               // Stores the topic being sent

WiFiClient espClient;
PubSubClient client(mqtt_server, 1883, espClient);
String clientName;

void setup()
{
  /* Initializing OneWire Sensors */
  TempSensors_init();

#if DEBUG // ----------
  Serial.begin(9600);
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
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
#endif // ----------

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
    delay(500);

#if DEBUG // ----------
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
#endif  // ----------

}

void loop()
{
  bool published;             // published succesfully flag
  char sensor[10];            // sensor topic variable

  //check for MQTT Connection
  if (!client.connected())
    reconnect();
  client.loop();

  // Getting Sensor count
  oneWire_count = TempSensors_init();       // Getting number of sensors
  oneWire_struct TempSensor[oneWire_count]; // structure that holds the sensors
  // Creating a pointer to pass in the full array of structures
  oneWire_struct *pointer = &TempSensor[0];
  // Getting temperature
  TempSensors_getTemp(&pointer);

  /* Publishing number of sensors */
  sprintf(topic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Status", "DeviceCount");
  published = client.publish(topic, String(oneWire_count).c_str());

#if DEBUG // -------------------
  if (published) Serial.println("\nTopic: " + String(topic) + " Data: " + String(oneWire_count));
  else           Serial.println("Topic: " + String(topic) + " FAILED");
#endif  // ---------------------

  /* Publishing Sensor Data */
  for (uint8_t i = 0; i < oneWire_count; i++)
  {
    sprintf(sensor, "%s%d", "Sensor", i + 1);
    /* Publishing number each temp data */
    sprintf(topic, "%s/%s/%s/%s/%s/%s", location, systemName, subSystem, transducerType, "Data" , sensor);
    published = client.publish(topic, String(TempSensor[i].value).c_str());

#if DEBUG // -------------------
    if (published) Serial.println("Topic: " + String(topic) + " Data: " + String(TempSensor[i].value));
    else           Serial.println("Topic: " + String(topic) + " FAILED");
#endif  // ---------------------
  }

  delay(5000);
}

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
    else
    {
#if DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
#endif
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

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
