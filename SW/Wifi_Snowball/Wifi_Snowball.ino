#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include "time.h"
#include "logo.h"
#include "web.h"
#include "lcd.h"
#include "WPS.h"

const char* host       = "esp32";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

bool oneshot = true;

hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile uint32_t isrCounter = 0;
volatile uint32_t lastIsrAt = 0;

// Option 1: use any pins but a little slower
Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN); 

// declare size of working string buffers. Basic strlen("d hh:mm:ss") = 10
const size_t    MaxString               = 50;

char timeStringBuff[MaxString]; //50 chars should be enough

// the string being displayed on the SSD1331 (initially empty)
char oldTimeString[MaxString]           = { 0 };

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

WebServer server(80);

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  //isrCounter++;
  //lastIsrAt = millis();
  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
  printLocalTime();
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
//  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M", &timeinfo);
  //print like "const char*"
  //Serial.println(timeStringBuff);
  updatedrawtext(timeStringBuff, WHITE);
}

void loading_popup() {
  // has the time string changed since the last oled update?
  if (oneshot) {
    Serial.println("loading popup displayed");

    tft.drawRGBBitmap(39, 15, logo, pngwing_com_width, pngwing_com_height);
    
    // home the cursor
    tft.setCursor(5,80);
    
    // change the text color to foreground color
    tft.setTextColor(WHITE, BLACK);
    tft.setTextSize(2);
    
    // draw the new time value
    tft.print("Loading...");
    
    oneshot = false;
  }
}

void updatedrawtext(char *text, uint16_t color) {
  String temp = text;
  int index;
  // has the time string changed since the last oled update?
  if (strcmp(text,oldTimeString) != 0) {
    Serial.println("update lcd");
 
    // home the cursor
    tft.setCursor(5,70);
    
    // change the text color to foreground color
    tft.setTextColor(color, BLACK);
    tft.setTextSize(4);
    
    // draw the new time value
    index = temp.indexOf(":");
    tft.print(temp.substring(index-2,index+3));
    // home the cursor
    tft.setCursor(0,110);
    
    // change the text color to foreground color
    tft.setTextColor(color, BLACK);
    tft.setTextSize(1);

    index = temp.length();
    // draw the new time value
    tft.print(temp.substring(0,index-10));
    
    // and remember the new value
    strcpy(oldTimeString,text);
  }
}

void lcdTestPattern(void)
{
  static const uint16_t PROGMEM colors[] =
    { RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, BLACK, WHITE };

  for(uint8_t c=0; c<8; c++) {
    tft.fillRect(0, tft.height() * c / 8, tft.width(), tft.height() / 8,
      pgm_read_word(&colors[c]));
  }
}

void beep(int repeat)
{
  for(int i=0; i<repeat;i++){
    ledcWrite(ledChannel, 100);
    delay(200);
    ledcWrite(ledChannel, 0);
  }
}

void setup() {
  char temp[MAXCHAR];
  String temp1;
  char ssid[MAXCHAR];
  char password[MAXCHAR];
  
  Serial.begin(115200);
  Serial.println(VERSION);
  
  tft.begin();
  tft.fillScreen(BLACK);
  delay(1000);
  tft.setRotation(2);
  delay(1000);
  loading_popup();

  if (!EEPROM.begin(EEPROM_SIZE))
  {
    Serial.println("failed to initialise EEPROM"); delay(1000000);
  }
  //Serial.println(eeprom_read(0));
  //Serial.println(eeprom_read(1));

  // return value change to char* ???
  temp1 = eeprom_read(0);
  temp1.toCharArray(ssid, temp1.length()+1);
  temp1 = eeprom_read(1);
  temp1.toCharArray(password, temp1.length()+1);

  //Serial.println(temp1);
  //Serial.println(ssid);
  
  if(ssid[0] == (byte)0){
    //set WPS 
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    Serial.println("Starting WPS");
    
    wpsInitConfig();
    esp_wifi_wps_enable(&config);
    esp_wifi_wps_start(0);
  
    while (WiFi.status() != WL_CONNECTED) {
      delay(350);
      Serial.print("$");
    }
    
    eeprom_write(0, WiFi.SSID());
    eeprom_write(1, WiFi.psk());
  
    Serial.println(eeprom_read(0));
    Serial.println(eeprom_read(1));
  
  } else {
    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(350);
      Serial.print(".");
    }
  }
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());

  // configure LED PWM functionalitites
  ledcSetup(ledChannel, freq, resolution);
  // attach the channel to the GPIO to be controlled
  ledcAttachPin(15, ledChannel);
  
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);

  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);

  // Set alarm to call onTimer function every second (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 1000000, true);

  // Start an alarm
  timerAlarmEnable(timer);
  
  //init and get the time
  configTime(gmtOffset_sec * 9, daylightOffset_sec, ntpServer);

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      timerAlarmDisable(timer);
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
  beep(1);
}

void loop() {
  // If Timer has fired
  if (xSemaphoreTake(timerSemaphore, 0) == pdTRUE){
    uint32_t isrCount = 0, isrTime = 0;
    // Read the interrupt count and time
    portENTER_CRITICAL(&timerMux);
    isrCount = isrCounter;
    isrTime = lastIsrAt;
    portEXIT_CRITICAL(&timerMux);
    /*
    // Print it
    Serial.print("onTimer no. ");
    Serial.print(isrCount);
    Serial.print(" at ");
    Serial.print(isrTime);
    Serial.println(" ms");
    */
  }
  server.handleClient();
  delay(1);
}
