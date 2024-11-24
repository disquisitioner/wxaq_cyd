/*
 * Wxaq_cyd -- Weather information aggregator built using the ESP32-2432S028R, affectionately
 * known as the "Cheap Yellow Display (CYD) board".
 *

 * Retrieves the latest envirnomental sensor data recorded by my home-built weather station (WX)
 * and Air Quality (AQ) monitoring devices, both of which use Dweet.io to publish their most recent
 * readings.
 *
 * Designed to make it easy to deploy a glanceable weather and air quality monitoring station
 * anywhere there's power and WiFi, without needing any attached sensors.
 *
 * Author: David Bryant (david@disquiry.com)
 */

// Load local configuration information, including hardware connections
#include "config.h"
#include "secrets.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include "time.h"

// Note: the ESP32 has 2 SPI ports, to have ESP32-2432S028R work with the TFT and Touch
// on different SPI ports each needs to be defined and passed to the library
SPIClass hspi = SPIClass(HSPI);
SPIClass vspi = SPIClass(VSPI);

#include "Adafruit_ILI9341.h"
Adafruit_ILI9341 display = Adafruit_ILI9341(&hspi, TFT_DC, TFT_CS, TFT_RST);
// works without SPIClass call, slower
// Adafruit_ILI9341 display = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST, TFT_MISO);
#include <XPT2046_Touchscreen.h>
XPT2046_Touchscreen ts(XPT2046_CS,XPT2046_IRQ);

// Include fonts for display
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

typedef struct envData {
  // Weather station data
  float outdoorTempF;
  float indoorTempF;
  float barometer;
  float windgust;
  float rainfall;
  String wxTimestamp;
  uint32_t wxEpochTime;
  bool wxAvailable;

  // AQI Monitor data
  float aqi;
  float vocIndex;
  float humidity;
  float aqTempF;   // Internal monitor temperature
  String aqTimestamp;
  uint32_t aqEpochTime;
  bool aqAvailable;

  String currentTimestamp;
  uint32_t currentEpochTime;  // To aid in checking for delayed data
} envData;
envData sensorData;

void setup() {
  Serial.begin(115200);
  delay(1000);   // Give Serial port time to connect (but works if no USB connection)
  // while (!Serial) {  // Beware: infinite loop if no USB connection at all
  //  delay(10);
  // }

  //  Initialize screen
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  display.begin();
  display.setRotation(screenRotation);
  display.setTextWrap(false);
  display.setTextColor(ILI9341_WHITE);
  display.setFont();
  display.setTextSize(2);
  display.fillScreen(SCREEN_BACKGROUND); // Erase screen to background color
  display.setCursor(10,10);
  display.print("Initializing...");


  // Connect to network via WiFi
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(", WiFi RSSI: ");
  Serial.println(WiFi.RSSI());

  // Configure time retrieval via NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Initialize key data storage attributes to indicate we don't yet have data
  sensorData.wxAvailable = false;
  sensorData.aqAvailable = false;
  sensorData.wxEpochTime = sensorData.aqEpochTime = sensorData.currentEpochTime = 0;

  display.setCursor(10,35);
  display.print("Connected!");
  display.setTextSize(1);
}

uint32_t counter = 1;

void loop() {
  // Try getting latest dweet for a test device
  HTTPClient http;
  StaticJsonDocument<400> dweetdoc;
  int httpResponseCode;
  String dweeturl, payload;
  DeserializationError error;

  // Load latest dweets (if available)
  Serial.println("Load latest WX dweet");
  loadWxDweet();
  Serial.println("Load latest AQ dweet");
  loadAqDweet();
  counter++;

  // Retrieve current local time from NTP
  fetchLocalTime();
  Serial.print("Time: "); Serial.print(sensorData.currentEpochTime);
  Serial.print(", WX delay = "); Serial.print(sensorData.currentEpochTime-sensorData.wxEpochTime);
  Serial.print(", AQ delay = "); Serial.println(sensorData.currentEpochTime-sensorData.aqEpochTime);

    // We have all the data & time info, so update the screen
  Serial.println("Update screen");
  updateScreen();
  
  delay(300000);
}

// This anticipates the day when there are multiple screens to cycle through
void updateScreen()
{
  // Basic listing of sensor data
  // dataScreen();

  // Fancier whole-screen layout
  fancyScreen();
}

// Retrieve the latest Weather Station dweet and extract relevant values therefrom.
// The Dweet URL and payload format are HIGHLY device specific so are hard coded here.
void loadWxDweet()
{
  HTTPClient http;
  StaticJsonDocument<400> dweetdoc;
  int httpResponseCode;
  String payload;
  DeserializationError error;

  // ******************************************************************************
  // Fetch Weather Station (WX) data from Dweet.io. (URL is defined in secrets.h)
  http.begin(WXDWEETURL);
  httpResponseCode = http.GET();
  // Serial.print("HTTP Response code: ");
  // Serial.println(httpResponseCode);

  // If we fetched a dweet, then process it
  if(httpResponseCode == HTTP_CODE_OK) {
    payload = http.getString();
    // Serial.println(payload);

    error = deserializeJson(dweetdoc,payload);
    if(error) {
      Serial.print("WX dweet deserialization failed: ");
      Serial.println(error.f_str());
    }
    else {
      JsonObject wxwith = dweetdoc["with"][0];
      const char* wxts = wxwith["created"];
      sensorData.wxTimestamp = String(wxts);
      JsonObject wxcontent = wxwith["content"];
      JsonArray wxsensors = wxcontent["sensors"];
      sensorData.outdoorTempF = wxsensors[0]["value"];
      sensorData.indoorTempF  = wxsensors[1]["value"];
      sensorData.barometer    = wxsensors[2]["value"];
      sensorData.windgust     = wxsensors[4]["value"];
      sensorData.rainfall     = wxsensors[6]["value"];
      sensorData.wxEpochTime  = parseTimestamp(sensorData.wxTimestamp);
      
      Serial.print("WX#"); Serial.print(counter); Serial.print(": ");
      Serial.print(sensorData.outdoorTempF,1); Serial.print("F, ");
      Serial.print(sensorData.indoorTempF,1); Serial.print("F, ");
      Serial.print(sensorData.barometer,2);  Serial.print(" in, ");
      Serial.print(sensorData.windgust,1);  Serial.print(" MPH, ");
      Serial.print(sensorData.rainfall,2);   Serial.print(" in");
      Serial.print("  @ ");   Serial.print(sensorData.wxTimestamp);
      Serial.print(" [");  Serial.print(sensorData.wxEpochTime); Serial.println("]");

      sensorData.wxAvailable = true;
    }
  }
  http.end();  // Clean up
}

// Retrieve the latest Air Quality monitor dweet and extract relevant values therefrom.
// The Dweet URL and payload format are HIGHLY device specific so are hard coded here.
void loadAqDweet()
{
  HTTPClient http;
  StaticJsonDocument<400> dweetdoc;
  int httpResponseCode;
  String payload;
  DeserializationError error;

  // ******************************************************************************
  // Fetch AQI monitor from Dweet.io.  (URL is defined in secrets.h)
  http.begin(AQDWEETURL);
  httpResponseCode = http.GET();
  // Serial.print("HTTP Response code: ");
  // Serial.println(httpResponseCode);

  // If we retrieved a dweet, process it
  if(httpResponseCode = HTTP_CODE_OK) {
    payload = http.getString();
    // Serial.println(payload);

    error = deserializeJson(dweetdoc,payload);
    if(error) {
      Serial.print("AQ dweet deserialization failed: ");
      Serial.println(error.f_str());
    }
    else {
      JsonObject withobj = dweetdoc["with"][0];
      const char* aqts = withobj["created"];
      sensorData.aqTimestamp = String(aqts);
      JsonObject contentobj = withobj["content"];
      sensorData.aqTempF  = contentobj["temperature"];
      sensorData.aqi      = contentobj["AQI"];
      sensorData.humidity = contentobj["humidity"];
      sensorData.vocIndex = contentobj["vocIndex"];
      sensorData.aqEpochTime  = parseTimestamp(sensorData.aqTimestamp);


      Serial.print("AQ#"); Serial.print(counter); Serial.print(": ");
      Serial.print(sensorData.aqTempF);  Serial.print(" F, ");
      Serial.print(sensorData.humidity); Serial.print(" %RH, ");
      Serial.print(sensorData.aqi);      Serial.print(" AQI, ");
      Serial.print(sensorData.vocIndex); Serial.print(" VOC");
      Serial.print("  @ ");              Serial.print(sensorData.aqTimestamp);
      Serial.print(" [");  Serial.print(sensorData.aqEpochTime); Serial.println("]");
      
      sensorData.aqAvailable = true;
    }
  }
  http.end();  // Clean up
}

// Display the data in a simple table, nothing fancy
void dataScreen()
{
  display.fillScreen(SCREEN_BACKGROUND);  // Clear screen to background color

  display.setCursor(10,10);
  display.print("  Outdoors: "); display.print(sensorData.outdoorTempF,1); display.print("F");
  display.setCursor(10,30);
  display.print("   Indoors: "); display.print(sensorData.indoorTempF,1); display.print("F");
  display.setCursor(10,50);
  display.print("  Humidity: "); display.print(sensorData.humidity,1); display.print("%");
  display.setCursor(10,70);
  display.print(" Barometer: "); display.print(sensorData.barometer,2); display.print("in HG");
  display.setCursor(10,90);
  display.print("       AQI: "); display.print(sensorData.aqi,1);
  display.setCursor(10,110);
  display.print(" Wind Gust: "); display.print(sensorData.windgust,1); display.print(" MPH");
  display.setCursor(10,130);
  display.print("Rain today: "); display.print(sensorData.rainfall,2); display.print(" in");
  display.setCursor(10,150);
  display.print(" VOC Index: "); display.print(sensorData.vocIndex,0);

  display.setTextSize(1);
  display.setCursor(0,210);
  display.print(sensorData.wxTimestamp);
  display.setTextSize(2);
}

void fancyScreen()
{
  int ival;
  int16_t x, y, w, h;
  
  display.fillScreen(SCREEN_BACKGROUND);  // Clear screen to background color

  // Top row of large digits for Outdoor Temperature, Indoor Temperature and AQI
  display.setTextColor(ILI9341_WHITE);
  display.setTextSize(1);
  display.setFont(&FreeSans24pt7b);
  ival = (0.5+sensorData.outdoorTempF);
  if(ival < 100) display.setCursor(25,40);
  else display.setCursor(10,40);
  display.print(ival);
  display.setFont(&FreeSans9pt7b);
  display.print(" F");

  display.setFont(&FreeSans24pt7b);
  ival = (0.5 + sensorData.indoorTempF);
  if(ival < 100) display.setCursor(140,40);
  else display.setCursor(125,40);
  display.print(ival);
  display.setFont(&FreeSans9pt7b);
  display.print(" F");

  // Display AQI as an integer. Adjust position based number of digits it contains
  display.setFont(&FreeSans24pt7b);
  ival = (0.5 + sensorData.aqi);
  if(ival < 10)       display.setCursor(260,40);
  else if(ival < 100) display.setCursor(250,40);
  else                display.setCursor(240,40);
  display.print(ival);

  // Middle row of medium-large digits for Humidity, Barometer, and Wind Gust
  display.setFont(&FreeSans18pt7b);
  display.setCursor(27,120);
  ival = (0.5 + sensorData.humidity);
  display.print(ival);  // Humidity as an (int)
  display.setFont(&FreeSans9pt7b);
  display.print(" %");

  display.setFont(&FreeSans18pt7b);
  display.setCursor(125,120);
  display.print(sensorData.barometer,2);
  // Adjust Wind Gust value position based on number of digits
  if(sensorData.windgust < 10) display.setCursor(250,120);
  else                         display.setCursor(240,120);
  display.print(sensorData.windgust,1);

  // Third row of medium-large digits for Rainfall (today) and VOC Index
  display.setCursor(17,195);
  // Show two decimal digits of rain if < 10 inches
  if(sensorData.rainfall < 10) display.print(sensorData.rainfall,2);
  else display.print(sensorData.rainfall,1);
  // Adjust VOC Index value position based on number of digits
  int vocii = 0.5 + sensorData.vocIndex;
  if(vocii < 10)       display.setCursor(160,195);
  else if(vocii < 100) display.setCursor(150,195);
  else                 display.setCursor(140,195);
  display.print(sensorData.vocIndex,0);

  // Now all the labels
  display.setFont(&FreeSans9pt7b);
  display.setTextColor(ILI9341_DARKGREY);
  display.setCursor(18,62);
  display.print("Outdoor");
  display.setCursor(140,62);
  display.print("Indoor");
  display.setCursor(255,62);
  display.print("AQI");

  display.setCursor(12,142);
  display.print("Humidity");
  display.setCursor(135,142);
  display.print("Pressure");
  display.setCursor(250,142);
  display.print("Wind")
  ;

  display.setCursor(35,217);
  display.print("Rain");
  display.setCursor(145,217);
  display.print("VOC-I");

  display.setFont();
  display.setTextSize(1);
  display.setCursor(190,230);
  display.print(sensorData.currentTimestamp);
  display.setTextColor(ILI9341_WHITE);

  // Check for delays in WX and/or AQ data reporting.  If either's dweet is older
  // than 30 minutes give an indication on screen so the user can investigate
  if((sensorData.currentEpochTime - sensorData.wxEpochTime) > dataDelayAlert_sec) {
    // WX data delayed!
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(ILI9341_RED);
    display.setCursor(250,185);
    display.print("WX*");
    display.setTextColor(ILI9341_WHITE);

  }
  if((sensorData.currentEpochTime - sensorData.aqEpochTime) > dataDelayAlert_sec) {
    // AQ data delayed!
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(ILI9341_RED);
    display.setCursor(250,205);
    display.print("AQ*");
    display.setTextColor(ILI9341_WHITE);
  }
}

// Parse an ISO8601 timestamp and return UNIX epoch time (seconds since 1/1/1970)
uint32_t parseTimestamp(String tstamp)
{
  uint16_t year, month, day, hour, minute;
  struct tm ts_tm;

  // ISO8601 timestamp looks like: 2024-11-19T21:07:53.510Z"
  sscanf(tstamp.c_str(),"%d-%d-%dT%d:%d",&year,&month,&day,&hour,&minute);

  uint32_t utime;
  utime = epochTime(year,month,day,hour,minute);

  /* Debugging...
  Serial.print(year);  Serial.print(",");
  Serial.print(month); Serial.print(",");
  Serial.print(day);   Serial.print(",");
  Serial.print(hour);  Serial.print(",");
  Serial.print(minute); Serial.print(" : ");
  Serial.println(utime);
  */

  return(utime);
}

void fetchLocalTime(){
  struct tm tnow;
  char cstr[80];

  if(!getLocalTime(&tnow)){
    Serial.println("Failed to obtain time");
    sensorData.currentEpochTime = 0;
    return;
  }
  sensorData.currentEpochTime = epochTime(tnow.tm_year+1900,tnow.tm_mon+1,tnow.tm_mday,tnow.tm_hour,tnow.tm_min);

  strftime(cstr,80,"%a %b %d %H:%M GMT",&tnow);
  sensorData.currentTimestamp = String(cstr);
  // Serial.println(sensorData.currentTimestamp);
}

//                 J  F  M  A  M   J   J   A   S  O    N   D
uint16_t doty[] = {0,31,59,90,120,151,181,212,243,273,304,334};
uint32_t epochTime(uint16_t year, uint16_t month, uint16_t day, uint16_t hour, uint16_t minute)
{
  uint32_t utime;
  uint16_t ldays;

  utime = (year - 1970)*365*24*60*60;
  utime += (doty[(month-1)] + (day-1))*24*60*60;
  ldays = (year - 1969)/4;   // Account for leap years since epoch
  if( ((year%4)==0) && (month > 2) ) ldays++;   // Add leap day for current year?
  utime += ldays*24*60*60;
  utime += ((hour*60)+minute)*60;
  return(utime);
}