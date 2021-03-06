#include "arduino_secrets.h"
//Specify gsm modem
#define TINY_GSM_MODEM_SIM800
//If You want to see GSM debug information just uncoment line under this
// #define TINY_GSM_DEBUG SerialMon
//Increase gsm rx buffer when working with internet. You can check which size of buffor is best for You
#define TINY_GSM_RX_BUFFER 128

/*Set Autoband for gsm device. IMPORTANT! Usually it take largest value and I saw(on my device) that values more than 
19200 isn't correct when You work with http request because it is very slow. Check which one is best for You.*/
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 19200

//This is for debug but leave it
#define SerialMon Serial

//Used libraries
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>



//GPS connection
static const int RXPin = 11, TXPin = 10;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

//connection to apn
const char apn[]  = "plus";
const char gprsUser[] = "";
const char gprsPass[] = "";

//connection to server for example: "your-server-address.com"
const char server[] = SECRET_SERVER;
char resource[40];
const int  port = 80;
char lat[10];
char lng[10];

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
//The serial connection to the GMS device
SoftwareSerial SerialAT(2, 3); // RX, TX

//Objects to make http requests using GMS device
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
HttpClient http(client, server, port);

void setup() 
{
  //Serial monitor for output
  SerialMon.begin(9600);
  while(!SerialMon)
  {
    //wait
  }
  
  //GPS module
  ss.begin(9600);
  
  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  delay(3000);
  
  Serial.print("Initializing modem...");
  while (!modem.restart()) {
    Serial.print("Failed to initialize modem, delaying 10s and retrying");
    delay(10000);
    // restart autobaud in case GSM just rebooted
    TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    delay(10000);
  }
  
  delay(3000);
  //When turn off and on device fast there is a chance that it is still connected to gprs
  if (!modem.isGprsConnected())
  {//It is importatnt to not use .waitForNetwork if it is currently connected to network. Connecting again may give you some problems
    if (!modem.isNetworkConnected()) 
    {
      Serial.print(F("Waiting for network..."));
      while (!modem.waitForNetwork())
      {
        Serial.println(" fail");
        delay(10000);
      }
    }
    Serial.println(" OK");

    //Connecting to Your APN from Your sim
    Serial.print(F("Connecting to "));
    Serial.print(apn);
    while (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" fail");
      delay(10000);
      // return;
    }
  }
  else
  {
    Serial.print("\nGPRS status:");
    Serial.println(" connected");
  }
  Serial.println("Start:");
}

void loop() 
{  
  //When working with some SoftwareSerial You have to pick which one we already should listen
  ss.listen();
  while (ss.available() > 0)
  {
    gps.encode(ss.read());
    
    if (gps.location.isUpdated())
    {
      dtostrf(gps.location.lat(), 9, 6, lat);
      dtostrf(gps.location.lng(), 9, 6, lng);
      sprintf(resource, "/add.php?lat=%s&lng=%s", lat, lng);
      Serial.println(resource);
      Serial.println();
      SerialAT.listen();
      if (modem.isGprsConnected())
      {
        Serial.print(F("Performing HTTP GET request... "));
        int err = http.get(resource);
        if (err != 0) {
          Serial.println(F("failed to connect"));
        }
     
        int status = http.responseStatusCode();
        Serial.println(status);
        if (!status) {
          delay(10000);
        }
     
        http.stop();
        Serial.println("Thirty seconds delay delay...");
        smartDelay(30000);
      }
      else
      {
        if (!modem.isNetworkConnected()) 
        {
          Serial.print(F("Waiting for network..."));
          if (!modem.waitForNetwork())
          {
            Serial.println(" fail");
            delay(10000);
            return;
          }
        }
        Serial.print(F("Connecting to "));
        Serial.print(apn);
        if (!modem.gprsConnect(apn, gprsUser, gprsPass))
        {
          Serial.println(" fail");
          delay(10000);
          return;
        }
      }
      //gps.speed.kmph() //speed
    }
  }
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
