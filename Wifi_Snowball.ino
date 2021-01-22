#include <WiFi.h>
#include "time.h"

const char* ssid       = "NETGEAR06";
const char* password   = "71115925";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
uint8_t timeZone = 9;

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// setting PWM properties
const int freq = 5000;
//const int ledChannel = 0;
const int resolution = 8;

void setup()
{
   // configure LED PWM functionalitites
  ledcSetup(0, freq, resolution);
  ledcSetup(1, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(16, 0);
  ledcAttachPin(17, 1);
  
  Serial.begin(115200);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  led(250, true);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  beep(1,false);
  led(0, false);
  
  //init and get the time
  configTime(gmtOffset_sec * timeZone, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop()
{
  delay(1000);
  printLocalTime();
}

void beep(int repeat, bool shortorlong)
{
  uint8_t time;
  
  if(shortorlong)
    time = 100;
  else
    time = 250;
    
  for(int i=0; i<repeat;i++){
    ledcWrite(0, 200);
    delay(time);
    ledcWrite(0, 0);
  }
}

void led(int level, bool onoff)
{
  if(onoff){
    ledcWrite(1, level);
    Serial.printf("led is On");
  }
  else{
    ledcWrite(1, 0);
    Serial.printf("led is Off");
  }
}
