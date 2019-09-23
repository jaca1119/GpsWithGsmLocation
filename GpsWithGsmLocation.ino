//Specify gsm modem
#define TINY_GSM_MODEM_SIM800
//If You want to see GSM debug information just uncoment line under this
// #define TINY_GSM_DEBUG SerialMon
//Increase gsm rx buffer when working with internet. You can check which size of buffor is best for You
#define TINY_GSM_RX_BUFFER 128

/*Set Autoband for gsm device. IMPORTANT! Usually it take largest value and I saw(on my device) that values more than 
19200 isn't correct when You work with http request because it is very slow. Check which one is best for You.*/
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 9600

//This is for debug but leave it
#define SerialMon Serial

//Used libraries
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <TinyGsmClient.h>


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
char resource[44];
const int port = 80;
char lat[12];
char lng[12];


// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);
//The serial connection to the GSM device
SoftwareSerial SerialAT(2, 3); // RX, TX

//Objects to make http requests using GMS device
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);

void setup() 
{
  
  //Serial monitor for output
  SerialMon.begin(9600);
  while(!SerialMon)
  {
    //wait
  }
  // //it have to be after SerialMon.begin
  // sprintf(host, "Host: %s\r\n", SECRET_SERVER);
  
  //GPS module
  ss.begin(9600);
  
  // Set GSM module baud rate
  TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
  delay(3000);
  
  Serial.print("Initializing modem...");
  if (!modem.init()) {
    Serial.print("Failed to initialize modem, delaying 10s and retrying");
    delay(10000);
    // restart autobaud in case GSM just rebooted
    TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
    // delay(10000);
  }
}

void loop() 
{  
 
  setResourcesFromGps();
  
  //When working with two or more SoftwareSerial You have to pick which one arduino should listen
  SerialAT.listen();
  if (!modem.isNetworkConnected())
  {
    Serial.print(F("Waiting for network..."));
    if (!modem.waitForNetwork())
    {
      Serial.println(" fail with connecting to network");
      delay(10000);
      return;
    }
  }

  if (!modem.isGprsConnected())
  {
    //Connecting to Your APN from Your sim
    Serial.print(F("Connecting to "));
    Serial.println(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) 
    {
      Serial.println(" fail connecting to APN");
      delay(10000);
      return;
    }
  }

  if (!client.connected())
  {
    //Connecting to server
    SerialMon.print(F("Connecting to "));
    SerialMon.println(server);
    if (!client.connect(server, port))
    {
        SerialMon.println(" fail connecting to server");
        delay(10000);
        return;
    }
  }
  
  sendDataToServer();
  
  //TODO get resonse status
  /*int status = http.responseStatusCode();
  Serial.println(status);
  if (!status)
  {
    delay(10000);
  }*/
  
  long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000L) 
    {
      SerialMon.println(F(">>> Client Timeout !"));
      client.stop();
      delay(10000L);
      return;
    }
  }
  
  //wait for server response
  printServerResponse();
  
  //end connection with server
  client.stop();
  
  Serial.println("Thirty seconds delay...");
  smartDelay(30000);
}

bool setResourcesFromGps()
{
  bool isResourcesSet = false;
  ss.listen();
  while (ss.available() > 0)
  {
    gps.encode(ss.read());
    
    if (gps.location.isUpdated())
    {
      Serial.println("Location updated.");
      dtostrf(gps.location.lat(), 9, 6, lat);
      dtostrf(gps.location.lng(), 9, 6, lng);
      sprintf(resource, "/location/add.php?lat=%s&lng=%s", lat, lng);
      Serial.println(resource);
      Serial.println(get);
      Serial.println();
      //gps.speed.kmph() //speed (double)
      //gps.course.deg() //course in degrees (double)
      isResourcesSet = true;
    }
  }
  
  return isResourcesSet;
}

void sendDataToServer()
{
  SerialAT.listen();
  Serial.println(F("Performing HTTP GET request... "));
  
  client.print("GET ");
  client.print(resource);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(SECRET_SERVER);
  client.print("\r\n");
  client.print("Connection: close\r\n\r\n");
}

void printServerResponse()
{
  while(client.connected()) 
  {
    while(client.available())
    {
      Serial.write(client.read());
    }
  }
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    // ss.listen();
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
