/*
  Date: July 15, 2018

  Small module for Temprature reading and reporting to Dissipator Unit
  Sends temperature and address using MQTT and HTTP protocol
  MQTT - using AERClient library for AER remote server
  HTTP - using espHTTP library for Heat Dissipator on local network

  Contact: Arthur Bondar
  Email:   arthur.bondar.1@gmail.com
*/

#include "AERClient.h"              // Custom made MQTT Class
#include <ESP8266WebServer.h>       // 
#include "temp_class.h"             // Custom made Temp Class
#include <ESP8266WiFi.h>            // ESP WiFi Libarary
#include <ESP8266HTTPClient.h>      // ESP HTTP client library

//
//  DEFINITIONS
//
#define DEVICE_ID     1             // 1 - Encom 2 - ThermoDynamics
#define BUFFSIZE      80            // Size for all character buffers
#define AP_SSID       "Tank1"       // Name of soft-AP network
#define DELIM         ":"           // Delimeter for serial communication
// Pin defines
#define BUTTON        14            // Interrupt to trigger soft AP mode
#define STATUS_LIGHT  13            // Light to indicate that HTTP server is running
#define REPORT_S      30            // Send out data ever number if seconds
#define MAX_SENSORS   30
// Dallas_Library
#define ONE_WIRE_GPIO 2             // Data pin of the bus is connected to pin 4

//  Wifi setup
char ssid[BUFFSIZE];                // Wi-Fi SSID
char password[BUFFSIZE];            // Wi-Fi password
char ip[BUFFSIZE] = "192.168.4.1";  // Default soft-AP ip address
char buf[BUFFSIZE];                 // Temporary buffer for building messages on display
volatile bool apMode = false;       // Flag to dermine current mode of operation
volatile bool newAP = false;
volatile int loop_count = 0;
volatile uint8_t oneWire_count = 0;
volatile uint8_t result = 0;
volatile uint32_t t_report = 0;

// Create Library Object password
AERClient aerClient(DEVICE_ID);     // publishing to IoT cloud
ESP8266WebServer server(80);        // webServer
// Setting up the interface for OneWire communication
Temp_Class Sensors(ONE_WIRE_GPIO);

// HTTP //
HTTPClient http;

// ------------------SETUP--------------------//
void setup()
{
  /* 
  // COM
  Serial.begin(38400);
  Serial.println("\n--- START ---");
  */

  // GPIO
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(STATUS_LIGHT, OUTPUT);
  digitalWrite(STATUS_LIGHT, LOW);

  // Attemp to connect to AP and print status
  result = aerClient.init();
  aerClient.mqtt_reconnect_retry = 2;
  //printWiFiStatus(result);
}

void loop()
{
  int httpCode;
  bool published;             // published succesfully flag
  String httpReturn;

  // Adjusting the timer in case of overflow
  if (millis() < t_report) t_report = millis();
  
  //  WiFi connected
  if (!apMode && (millis() - t_report > REPORT_S * 1000))
  {
    t_report = millis();        // Update counter
    
    // HTTP Setup //
    http.begin("192.168.2.111", 1880, String("/Tank"+String(DEVICE_ID)));
    http.addHeader("Content-Type", "text/plain");

    // Getting Sensor count
    oneWire_count = Sensors.init();           // Getting number of sensors
    if(oneWire_count > MAX_SENSORS) return;
    oneWire_struct TempSensor[oneWire_count];
    // Creating a pointer to pass in the full array of structures
    oneWire_struct *pointer = &TempSensor[0];
    Sensors.getTemp(&pointer);            // Getting temperature

    // Publishing number of sensors MQTT
    sprintf(buf, "%d/%s/%s", DEVICE_ID, "Status", "DeviceCount");
    published = aerClient.publish(buf, String(oneWire_count).c_str());
    //  HTTP
    httpCode = http.POST("DeviceCount:" + String(oneWire_count));
    httpReturn = http.getString();

    /*
    // MQTT
    if (published)  Serial.println("Topic: " + String(buf) + " Data: " + String(oneWire_count));
    else            Serial.println("Topic: " + String(buf) + " FAILED");
    // HTTP
    Serial.println("HTTP Code: " + String(httpCode));
    Serial.println("HTTP return: " + String(httpReturn));
    */
    
    // Publishing Sensor Data 
    for (uint8_t i = 0; i < oneWire_count; i++)
    {
      // MQTT
      sprintf(buf, "%d/%s/%d", DEVICE_ID, "Data" , i + 1);
      published = aerClient.publish(buf, String(TempSensor[i].value).c_str());
      // HTTP
      httpCode = http.POST("Sensor" + String(i + 1) + ":" + String(TempSensor[i].value));
      httpReturn = http.getString();

      /*
      // MQTT
      if (published) Serial.println("Topic: " + String(buf) + " Data: " + String(TempSensor[i].value));
      else           Serial.println("Topic: " + String(buf) + " FAILED");
      // HTTP
      Serial.println("HTTP Code: " + String(httpCode));
      Serial.println("HTTP return: " + String(httpReturn));
      */
      delay(100);
    }/////
    http.end();  //Close connection
  }
  //  AP Mode
  else
  {
    server.handleClient();
    // New Ap was set
    if (newAP)
    {
      digitalWrite(STATUS_LIGHT, LOW);
      apMode = false; newAP = false;
      // Attemp to connect to AP and print status
      result = aerClient.init(ssid, password);
      //printWiFiStatus(result);
    }
  }

  //
  //  Switch to AP mode
  //
  if (!digitalRead(BUTTON) && !apMode)
  {
    delay(10);
    //Serial.println("\n\n\n -- Setting soft-AP -- ");
    // LED indicator ON
    digitalWrite(STATUS_LIGHT, HIGH);
    // Disconnect from MQTT
    aerClient.disconnect();
    WiFi.mode(WIFI_AP_STA); // WiFi.mode(WIFI_AP_STA);
    // Starting softAP
    if (!WiFi.softAP(AP_SSID))
      return;
    //Serial.println("Soft AP Ready");

    // Handlers
    server.on("/", handleRoot);
    server.on("/success", handleSubmit);
    server.on("/inline", []() {
      server.send(200, "text/plain", "this works without need of authentification");
    });
    server.onNotFound(handleNotFound);
    //here the list of headers to be recorded
    const char * headerkeys[] = {
      "User-Agent", "Cookie"
    } ;
    size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
    //ask server to track these headers
    server.collectHeaders(headerkeys, headerkeyssize );
    server.begin();

    // setting status True
    apMode = true;
  }

  // must be > 100ms to give time for WiFi
  delay(100);
}


//------------------------------------------------------
// Button handler
// Sets AP mode
//-------------------------------------------------------
void handleButton()
{

}

//------------------------------------------------------
// Page to inform the user they successfully changed
// Wi-Fi credentials
// @param none
// @return void
//-------------------------------------------------------
void handleSubmit() {
  String content = "<html><body><H2>WiFi information updated!</H2><br>";
  server.send(200, "text/html", content);
  delay(2000);
  // Shutdown routine
  digitalWrite(STATUS_LIGHT, LOW);
  WiFi.softAPdisconnect(); delay(500);
  WiFi.mode(WIFI_OFF); delay(500);
  newAP = true; // Flag that we received new AP parameters
}

//------------------------------------------------------
// Page to enter new Wi-Fi credentials
// @param none
// @return void
//-------------------------------------------------------
void handleRoot() {
  String htmlmsg;
  if (server.hasArg("SSID") && server.hasArg("PASSWORD"))
  {
    server.arg("SSID").toCharArray(ssid, BUFFSIZE);
    server.arg("PASSWORD").toCharArray(password, BUFFSIZE);
    newAP = true; // Flag that we received new AP parameters

    server.sendContent("HTTP/1.1 301 OK\r\nLocation: /success\r\nCache-Control: no-cache\r\n\r\n");
    return;
  }
  String content = "<html><body><form action='/' method='POST'>Please enter new SSID and password.<br>";
  content += "SSID:<input type='text' name='SSID' placeholder='SSID'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + htmlmsg + "<br>";
  server.send(200, "text/html", content);
}

//------------------------------------------------------
// Page displayed on HTTP 404 not found error
// @param none
// @return void
//-------------------------------------------------------
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  server.send(404, "text/plain", message);
}

//------------------------------------------------------
// Printf WiFi result code the LCD and over Serial
//-------------------------------------------------------
void printWiFiStatus (uint8_t status)
{
  switch (status)
  {
    case WL_CONNECTED:
      Serial.println("Wifi Connected, SSID: " + WiFi.SSID() + " PSK: " + WiFi.psk());
      break;
    case WL_NO_SSID_AVAIL:
      Serial.println("SSID not found, SSID: " + WiFi.SSID() + " PSK: " + WiFi.psk());
      break;
    case WL_CONNECT_FAILED:
      Serial.println("Wrong password, SSID: " + WiFi.SSID() + " PSK: " + WiFi.psk());
      break;
    default:
      Serial.println("WiFi error, SSID: " + WiFi.SSID() + " PSK: " + WiFi.psk());
      break;
  }
}
