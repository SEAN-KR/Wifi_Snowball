#include <WiFi.h>
#include "time.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>

const char* ssid       = "NETGEAR06";
const char* password   = "71115925";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// Screen dimensions
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 128 // Change this to 96 for 1.27" OLED.

// You can use any (4 or) 5 pins 
#define SCLK_PIN 18 //2
#define MOSI_PIN 23 //3
#define DC_PIN   17 //4
#define CS_PIN   5  //5
#define RST_PIN  16 //6

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF


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

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  isrCounter++;
  lastIsrAt = millis();
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
  strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  //print like "const char*"
  //Serial.println(timeStringBuff);
  updatedrawtext(timeStringBuff, WHITE);
}

void updatedrawtext(char *text, uint16_t color) {
  String temp = text;
  // has the time string changed since the last oled update?
  if (strcmp(text,oldTimeString) != 0) {
    Serial.println("update lcd");
    // home the cursor
    tft.setCursor(7,40);
    
    // change the text color to foreground color
    tft.setTextColor(color, BLACK);
    tft.setTextSize(4);
    
    // draw the new time value
    tft.print(temp.substring(24,29));
    // home the cursor
    tft.setCursor(5,80);
    
    // change the text color to foreground color
    tft.setTextColor(color, BLACK);
    tft.setTextSize(1);
    
    // draw the new time value
    tft.print(temp.substring(0,18));
    
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
  Serial.begin(115200);

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

   //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  beep(1);
  
  //init and get the time
  configTime(gmtOffset_sec * 9, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF); //connect to WiFi

  tft.begin();
//  lcdTestPattern();
  delay(1000);
  tft.fillScreen(BLACK);
  delay(1000);
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
}
