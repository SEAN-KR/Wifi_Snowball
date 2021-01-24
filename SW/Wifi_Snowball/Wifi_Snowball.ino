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

#define pngwing_com_width 50
#define pngwing_com_height 50
// 'pngegg', 50x50px
// 'pngegg', 50x50px
const uint16_t myBitmap [] PROGMEM = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1041, 0x0841, 0x0860, 0x1060, 0x0860, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0801, 0x1061, 0x3122, 0x5a02, 0x6a83, 0x5a03, 0x3102, 0x0841, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20c1, 0x5a23, 0xc445, 0xe4e4, 0xe504, 0xdce4, 0x9b84, 
  0x4182, 0x0021, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0020, 0x49c2, 0xb3e4, 0xdce4, 0xcc84, 0xc444, 
  0xbc04, 0xabc4, 0x6223, 0x1882, 0x1061, 0x1041, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0008, 0x1061, 0x49a2, 0x8b24, 0x7263, 
  0x51c2, 0x4162, 0x3962, 0x4182, 0x4182, 0x3962, 0x3922, 0x2902, 0x20c1, 0x20c1, 0x20a1, 0x1881, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0865, 0x1084, 0x1082, 0x20e1, 
  0x49a2, 0x7283, 0x9b64, 0xb404, 0xcc64, 0xd4a4, 0xd4a4, 0xd4a4, 0xd484, 0xc444, 0xabe4, 0x8b03, 0x6223, 0x3942, 0x28e1, 0x20c1, 
  0x0861, 0x0001, 0x1082, 0x1882, 0x1062, 0x0842, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0822, 0x1883, 0x3123, 
  0x6a63, 0xabe4, 0xd4a4, 0xe504, 0xe504, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xe504, 0xdce4, 0xc444, 
  0x8b23, 0x51c2, 0x28e1, 0x0841, 0x3102, 0x49a3, 0x4183, 0x20c2, 0x1062, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x0842, 0x28e2, 
  0x6223, 0xb404, 0xdce4, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xe504, 0xcca4, 0x8b23, 0x3962, 0x6223, 0xbc04, 0xd4a5, 0x9364, 0x3122, 0x1042, 0x0005, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x0862, 
  0x3142, 0x8b03, 0xd4a4, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xbc24, 0x6223, 0x5a03, 0xb3e4, 0xe4e4, 0x9324, 0x20c2, 0x0001, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x1061, 0x3962, 0x9b84, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xcc84, 0x6a63, 0x51e3, 0xb3e4, 0xc444, 0x3122, 
  0x0841, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0861, 0x3142, 0x9b64, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4a4, 0x6a63, 0x6223, 
  0x9b64, 0x3122, 0x0841, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0841, 0x28e2, 0x82e3, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xcc84, 0x6223, 0x3962, 0x1081, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x1881, 0x51e3, 0xcca4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xe4e4, 0xbc04, 0x3942, 0x0841, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0841, 0x2902, 0x9b84, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0x8b03, 0x28e2, 0x0801, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1881, 0x51c3, 0xd4a4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4a4, 0xb404, 0xa3a3, 0xb3e3, 0xbc44, 0xcc84, 0xd4a4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xcc64, 0x4182, 0x1081, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x20c1, 0x8303, 0xe504, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xc464, 0x7ac3, 0x4182, 0x49c2, 0x5a02, 0x6a63, 0x82e3, 0x9b64, 0xc444, 0xdcc4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0x7ac3, 0x20a1, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0842, 0x28e2, 0xb404, 0xe4e4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4c4, 0xbc24, 0x9b63, 0x82e3, 0x6a62, 0x59e2, 0x49a2, 0x51c2, 
  0xa383, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xb404, 0x28e2, 0x0861, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1061, 0x3942, 0xd484, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xe504, 0xdce4, 0xd4a4, 
  0xbc24, 0xabc4, 0xc444, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xcc84, 0xa3a3, 0xabc3, 0xbc24, 0xcc84, 0xdcc4, 0xd4a4, 
  0x4182, 0x1881, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1082, 0x51c2, 
  0xdce5, 0xdcc4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdcc4, 0xcc64, 
  0xbc04, 0xcc84, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xbc04, 0x6223, 0x49a2, 0x5a03, 0x7283, 
  0x8b03, 0xbc44, 0x5a03, 0x18a1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x1882, 0x6223, 0xe4e5, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xcc84, 0x82c3, 0x4982, 0x8b23, 0xd4a4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4a4, 0xabc4, 0x8b03, 
  0x6a63, 0x51e2, 0x49a2, 0x9b64, 0x7283, 0x20c1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x20a1, 0x7263, 0xe504, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xc464, 0x6223, 0x1881, 0x7283, 0xd4a4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xe4e4, 0xe4e4, 0xdcc4, 0xc464, 0xabc3, 0xc464, 0x8303, 0x20a1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x20a1, 0x6a63, 0xe504, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdcc4, 0xdcc4, 0xdcc4, 0xd4c4, 0xa3a4, 0x82e3, 0xb3e4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xc444, 0x9b63, 0xbc24, 0xdcc4, 0xe4e4, 0xe504, 0x8b24, 0x20c1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a1, 0x6243, 0xe4e4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdca5, 0xe446, 0xe407, 0xe407, 0xe446, 0xdc85, 0xdca4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0x7ac3, 0x2901, 0x6a63, 0xcc84, 0xdce4, 0xe504, 0x8b24, 0x20c1, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a1, 0x51e2, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdcc4, 0xe466, 0xeb8a, 0xeb2b, 0xeb2b, 0xeb4b, 0xeb89, 0xe427, 0xdca4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xe4e4, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0x82e3, 0x3141, 0x7283, 0xcc84, 0xdce4, 0xe504, 0x82e3, 0x20c1, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18a0, 0x4162, 0xd4a4, 0xdce4, 0xdcc4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe485, 0xe3c9, 0xeb6b, 0xeb2b, 0xeb2c, 0xeb2c, 0xeba9, 0xe485, 0xcc84, 0xabc3, 
  0xa3a3, 0xb403, 0xb404, 0xbc04, 0xcc84, 0xdce4, 0xdce4, 0xdce4, 0xc464, 0xa3a4, 0xc444, 0xdcc4, 0xdce4, 0xe504, 0x7283, 0x18a1, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0841, 0x2901, 0xbc04, 0xe4e4, 
  0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdcc4, 0xdc85, 0xe447, 0xe407, 0xe3e8, 0xe3e8, 0xe427, 0xbbe4, 
  0x7ac4, 0x6aa7, 0x6ae9, 0x6265, 0x4182, 0x4162, 0x6a42, 0xa383, 0xcc84, 0xdca5, 0xe466, 0xe447, 0xe466, 0xdca5, 0xdcc4, 0xdce4, 
  0x5a03, 0x18a1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20c1, 
  0x8b04, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdcc4, 0xdcc4, 0xdcc4, 0xe4c4, 
  0xcc44, 0x7ac4, 0x736d, 0xb596, 0xbe18, 0x8c71, 0x2945, 0x0000, 0x3165, 0x5a67, 0x72c4, 0xc3c6, 0xe38a, 0xeb4b, 0xeb8a, 0xe3a9, 
  0xe466, 0xcc84, 0x4162, 0x18a1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x1881, 0x51c2, 0xd4a4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xe4e4, 0xabc3, 0x6ac8, 0xb596, 0xffdf, 0xffff, 0xef5d, 0x9492, 0x632c, 0x9cf3, 0xb5b6, 0x6b2c, 0x82c6, 0xd388, 0xeb6a, 
  0xeb4b, 0xeb2b, 0xec08, 0xaba4, 0x2902, 0x0801, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0841, 0x2902, 0xa384, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xd4c4, 0xcc84, 0xd4a4, 0xaba3, 0x6ae8, 0xb5d7, 0xffff, 0xffff, 0xffff, 0xdedb, 0xd6ba, 0xf7be, 0xffff, 0xad75, 0x6286, 
  0xbbe4, 0xe466, 0xe427, 0xe427, 0xe486, 0x6a63, 0x20c1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x18a1, 0x59e3, 0xcca4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xd4a4, 0xa3a3, 0x7283, 0x6223, 0x6243, 0x6a63, 0x5204, 0x736d, 0xc638, 0xdf1c, 0xb5b6, 0x73af, 0xbdf7, 0xffdf, 0xffff, 
  0xad76, 0x6286, 0xbc23, 0xdce4, 0xdcc4, 0xe4e4, 0xbc24, 0x3942, 0x1061, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0841, 0x28e2, 0x82e3, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xd4a4, 0x9b64, 0x5a03, 0x7aa3, 0x9343, 0x8b23, 0x6243, 0x4183, 0x51e5, 0x62c8, 0x630a, 0x62a8, 0x49c4, 0x630b, 
  0xad55, 0xb596, 0x62eb, 0x7ae4, 0xcc84, 0xdce4, 0xdce4, 0xdce4, 0x7283, 0x20a2, 0x0002, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1061, 0x3962, 0xa384, 0xdce4, 0xdce4, 
  0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xb3e4, 0x6223, 0x8303, 0xcc84, 0xdcc4, 0xd4c4, 0xbc04, 0x6243, 0x6243, 0x9b63, 0xa384, 0xabc4, 
  0xabc4, 0x8304, 0x62a7, 0x6287, 0x7ac4, 0xbc24, 0xdcc4, 0xdce4, 0xe4e4, 0x9b84, 0x3942, 0x1061, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1081, 0x49a2, 
  0xabc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4a4, 0x8b03, 0x6223, 0xcc64, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0x9323, 0x6a63, 0xbc04, 
  0xdce4, 0xdce4, 0xdcc4, 0xcc84, 0xbc03, 0xb403, 0xcc64, 0xdcc4, 0xdce4, 0xe4e4, 0xb404, 0x49c2, 0x1881, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0002, 0x1882, 0x4182, 0x9b83, 0xdce4, 0xdce4, 0xdce4, 0xcc64, 0x72a3, 0x7ac3, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xa3a3, 
  0x6243, 0xabc3, 0xe4e4, 0xdce4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xb404, 0x51e3, 0x18a2, 0x0002, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0001, 0x1061, 0x3122, 0x7ac3, 0xcc84, 0xe504, 0xc444, 0x6a63, 0x8b23, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 
  0xe4e4, 0xabc3, 0x6a43, 0xa3a3, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xa3a4, 0x49c2, 0x18a2, 0x0801, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0002, 0x1882, 0x3942, 0x5a03, 0x5a02, 0x9b83, 0xabc4, 0x6a43, 0x9b63, 0xe4e4, 0xdce4, 
  0xdce4, 0xdce4, 0xe4e4, 0xabc4, 0x6a43, 0xa3a3, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xe4e4, 0xc444, 0x7ac3, 0x3962, 0x1881, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x2902, 0x8b03, 0xc444, 0x82e3, 0x51e3, 0x4162, 0x49a2, 0xa3a3, 
  0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xabc3, 0x6a43, 0xabc3, 0xe504, 0xe504, 0xe504, 0xdcc4, 0xbc24, 0x82e3, 0x49a2, 0x28e2, 
  0x1061, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3122, 0xabc4, 0xe504, 0xd4a4, 0xb404, 0x72a3, 
  0x49a2, 0xa3a3, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xe4e4, 0xa383, 0x6223, 0x9b64, 0xbc24, 0xa384, 0x7ac3, 0x5a02, 0x3122, 0x20c2, 
  0x1062, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x3942, 0xb3e4, 0xe504, 0xdce4, 
  0xdce4, 0xbc24, 0x8b03, 0xb404, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0x9343, 0x3922, 0x3942, 0x49a2, 0x51c3, 0x6a43, 0x8b04, 
  0x51c3, 0x20a1, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x3122, 0xa384, 
  0xcc64, 0xdcc4, 0xdce4, 0xdcc4, 0xd4a4, 0xdcc4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdcc4, 0x82e3, 0x6223, 0xa3a4, 0xbc24, 0xc444, 
  0xd484, 0xdcc4, 0x7ac3, 0x2922, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 
  0x3102, 0x6a64, 0x6a63, 0xcc84, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xcc64, 0x72a4, 0x8bcb, 0xdd4a, 
  0xdcc4, 0xdd06, 0xd4e6, 0xdcc4, 0x8303, 0x3122, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0001, 0x2922, 0x5a23, 0x5a03, 0xabc4, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xaba3, 0x6ac8, 
  0xb596, 0xeed5, 0xe5cd, 0xeed4, 0xacf0, 0x8325, 0x7283, 0x3942, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0002, 0x28e2, 0x5a03, 0x7283, 0x6223, 0xd4a4, 0xe4e4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xdce4, 0xd4a4, 
  0x7aa3, 0x7bad, 0xe71c, 0xffde, 0xf7bc, 0xffde, 0xad75, 0x72e7, 0x7263, 0x4162, 0x0001, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x1881, 0x51c2, 0xabc4, 0x51e3, 0x6a43, 0xc464, 0xe504, 0xe504, 0xe504, 0xe504, 
  0xd4c4, 0x82e2, 0x5a68, 0xbdd7, 0xffff, 0xffff, 0xffff, 0xffff, 0xb5b6, 0x72e7, 0x7263, 0x4182, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0021, 0x4182, 0xc444, 0xc444, 0x6223, 0x4182, 0x6243, 0x8b03, 
  0x9323, 0x7ac3, 0x51e3, 0x5aa9, 0xbdd7, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xb5b7, 0x7308, 0x7283, 0x4982, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x3142, 0xa3a4, 0xe505, 0xcc65, 0x7ac4, 
  0x20c1, 0x0840, 0x0840, 0x1081, 0x4a8a, 0xbe18, 0xffdf, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xad55, 0x72e7, 0x7283, 0x4182, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x8000, 0x8408, 0xbc08, 
  0xbc08, 0x8208, 0x0000, 0x0000, 0x0000, 0x0000, 0x4208, 0xbdf7, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xbdf7, 0x8208, 
  0x8200, 0x8000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
  0x0000, 0x0000, 0x0000, 0x0000
};

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

    tft.drawRGBBitmap(39, 0, myBitmap, pngwing_com_width, pngwing_com_height);
    
    // home the cursor
    tft.setCursor(5,70);
    
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
  // has the time string changed since the last oled update?
  if (strcmp(text,oldTimeString) != 0) {
    Serial.println("update lcd");
 
    // home the cursor
    tft.setCursor(5,60);
    
    // change the text color to foreground color
    tft.setTextColor(color, BLACK);
    tft.setTextSize(4);
    
    // draw the new time value
    tft.print(temp.substring(24,29));
    // home the cursor
    tft.setCursor(10,100);
    
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
  tft.fillScreen(BLACK);
  delay(1000);
  loading_popup();
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
