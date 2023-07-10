#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h> 
#include <WiFiManager.h> 
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WifiUDP.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <TJpg_Decoder.h>


// Include SPIFFS
#define FS_NO_GLOBALS
#include <FS.h>
#include "SPI.h"  

//host server
const char* host = "script.google.com";
const char *GScriptId = "AKfycbxKAaekPVHQYyC3ZawrfBqyl6isiUH_KxwzppJqQW5GxNZ9oK4CXUfFVUwaeS3xgVY";

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];

bool poke_status = false; // status variable to trigger LED notification

//GScript APIs
String url_read = String("/macros/s/") + GScriptId + "/exec?read";
String url_write = String("/macros/s/") + GScriptId + "/exec?write";
String url_pokeStatus = String("/macros/s/") + GScriptId + "/exec?pStatus";
String url_unpoke = String("/macros/s/") + GScriptId + "/exec?unpoke";
const uint16_t httpsPort = 443;
String current_data = "";

enum APIS {READ, WRITE, POKESTATUS, POKE, UNPOKE};


//response 
const char server[] = "https://api.openweathermap.org/";

//  "data/2.5/forecast?q=Bangalore,in&APPID=0c40676db9bcb51086ea89239551ca2b&units=metric&cnt=2"; 
// String SjcWeather = "data/2.5/forecast?q=San%20Jose,us&APPID=0c40676db9bcb51086ea89239551ca2b&units=metric&cnt=2";
String apiKey = "91343e3f6af3910f20db5c6181661994";

enum cities {BLR, SJC};
String sjc_weather_icon, blr_weather_icon;

#define JSON_BUFF_DIMENSION 2500

// Use WiFiClient class to create TCP connections
HTTPSRedirect* client = nullptr;
String result; 

//Init TFT display.
#define TFT_CS         15 // d8    
#define TFT_RST        4  // d2 
#define TFT_DC         0  // d3
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST); 



//Time variables
// Define NTP properties
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "ca.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

//data for date time
String date;
String t;
const char * days[] = {"Sun", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

bool scroll = false;

void getTime(){
    date = "";  // clear the variables
    t = "";
    
    // update the NTP client and get the UNIX UTC timestamp 
    timeClient.update();
    unsigned long epochTime =  timeClient.getEpochTime();

    // convert received time stamp to time_t object
    time_t local, utc;
    utc = epochTime;

    // Then convert the UTC UNIX timestamp to local time
    TimeChangeRule IST = { "IST", First, Sun, Jan, 0, +270};
    Timezone IndStd(IST);
    local = IndStd.toLocal(utc);

    // now format the Time variables into strings with proper names for month, day etc
    date += days[weekday(local)-1];
    date += ", ";
    date += months[month(local)-1];
    date += " ";
    date += day(local);
    date += ", ";
    date += year(local);

    // format the time to 12-hour format with AM/PM and no seconds
    t += hourFormat12(local);
    t += ":";
    if(minute(local) < 10)  // add a zero if minute is under 10
      t += "0";
    t += minute(local);
    t += " ";
    t += ampm[isPM(local)];
}

void blink_led(int pin= LED_BUILTIN){
    digitalWrite(pin, HIGH);
    delay(200);
    digitalWrite(pin, LOW);
    delay(300);
}

// void breathe_led(int pin= LED_BUILTIN){
//   float smoothness_pts = 500;//larger=slower change in brightness  
//   float gamma = 0.14; // affects the width of peak (more or less darkness)
//   float beta = 0.5; // shifts the gaussian to be symmetric

//   for (int ii=0;ii<smoothness_pts;ii++){
//     float pwm_val = 255.0*(exp(-(pow(((ii/smoothness_pts)-beta)/gamma,2.0))/2.0));
//     analogWrite(LED_BUILTIN,int(pwm_val));
//     delay(5);
//   }
// }

void breathe_led(int pin = LED_BUILTIN) {
  const unsigned long duration = 2000; // Total duration of the breathing effect in milliseconds
  const unsigned long interval = 10; // Update interval for changing brightness in milliseconds

  static unsigned long startTime = 0;

  if (startTime == 0) {
    startTime = millis(); // Set the start time when the function is first called
  }

  unsigned long elapsedTime = millis() - startTime;
  float progress = (float)elapsedTime / duration;
  
  if (progress >= 1.0) {
    startTime = 0; // Reset the start time for the next cycle
    progress = 0.0;
  }

  // Compute the brightness using a Gaussian function
  float smoothness_pts = 500;
  float gamma = 0.14;
  float beta = 0.5;
  float pwm_val = 255.0 * (exp(-(pow(((progress * smoothness_pts / 1000) - beta) / gamma, 2.0)) / 2.0));

  analogWrite(pin, int(pwm_val));

  // Delay until the next interval
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = currentMillis;
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    // Continue with the next iteration
  }
}


bool willStringfit(const String buf, int x, int y, int w, int h)
{
    int16_t x1, y1;
    uint16_t w1, h1;
    tft.getTextBounds(buf, x, y, &x1, &y1, &h1, &w1); //calc width of new string
    if((w1 > w) || (h1 > h))
      return false;
    return true;
}

void scrolltext(int x, int y, String s, uint8_t dw = 1, const GFXfont *f = NULL, int sz = 1)
{
    int16_t x1, y1, wid = tft.width(), inview = 1;
    uint16_t w, h;
    tft.setFont(f);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(sz);
    tft.setTextWrap(false);
    tft.getTextBounds(s, x, y, &x1, &y1, &w, &h);
    //    w = strlen(s) * 6 * sz;

    for (int steps = wid + w; steps >= 0; steps -= dw) {
        x = steps - w;
        if (f != NULL) {
            inview = wid - x;
            if (inview > wid) inview = wid;
            if (inview > w) inview = w;
            tft.fillRect(x > 0 ? x : 0, y1, inview + dw, h, BLACK);
        }
        x -= dw;
        tft.setCursor(x, y);
        tft.print(s);
        // if (f == NULL) tft.print("  "); //rubout trailing chars
        delay(1);
    }
}

void process_data(String data){
      //reset widget
      message_Widget();
      if(willStringfit(data,0,40, tft.width(), 1.5*tft.height()/5)){
        scroll = false;
        tft.setCursor(0, 40);    //Horiz/Vertic
        tft.setTextSize(1);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK); //rewrite
        tft.setTextWrap(true);
        tft.println(data);
        tft.setTextWrap(false);
      }
      else{
        // scroll = true;
        tft.setCursor(0, 40);    //Horiz/Vertic
        // scrolltext(0,40,data);
        tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
        tft.setTextWrap(true);
        tft.println(data);
        tft.setTextWrap(false);

      }
}

void display_time(){
      tft.setTextColor(ST77XX_BLACK, ST77XX_YELLOW);
      getTime();
      tft.setTextSize(2);
      tft.setCursor(16,5); //limiting it to within data_time widget
      tft.println(t);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_BLACK, ST77XX_YELLOW);
      tft.setCursor(13,20); //limiting it to within date_time widget
      // drawCentreString(date,0,20);
      tft.println(date);
  }

void date_time_Widget(){
    //create a rect in top 20% of display
      tft.fillRect(0, 0, tft.width() , tft.height()/5 , ST77XX_YELLOW);
  }

void message_Widget() {
  //create messenger_widget
  tft.drawLine(0, tft.height()/5 , tft.width(), tft.height()/5, ST77XX_BLACK);
  tft.fillRect(0, tft.height()/5, tft.width(), 1.5*tft.height()/5, ST77XX_BLACK);
  tft.drawLine(0, 2.5*tft.height()/5 , tft.width(), 2.5*tft.height()/5, ST77XX_WHITE);
  }

void poke_Widget() {
  //create messenger_widget
  tft.fillRect(0, 2.5*tft.height()/5+1, tft.width(), tft.height()/10, ST77XX_ORANGE);
  tft.drawLine(0, 3*tft.height()/5 , tft.width(), 3*tft.height()/5, ST77XX_WHITE);
  }

void poke_Event(bool reply = false){
    //if incoming poke change to green
    tft.fillRect(0, 2.5*tft.height()/5+1, tft.width(), tft.height()/10, ST77XX_GREEN);
    tft.setCursor(2,85);
    tft.setTextColor(ST77XX_BLACK, ST77XX_GREEN);
    tft.println("YOU'VE BEEN POKED :P");
    poke_status = true;
    //if outgoing change to sending poke animation 
    if(reply){
    tft.fillRect(0, 2.5*tft.height()/5+1, tft.width(), tft.height()/10, ST77XX_ORANGE);
    tft.setCursor(20,85);
    tft.setTextColor(ST77XX_BLACK, ST77XX_ORANGE);
    tft.println("POKED BACK xD");
    blink_led();
    poke_status = false;
    }
}

void get_data(int api){
 static bool flag = false; 
 client = new HTTPSRedirect(httpsPort);
 client->setInsecure();
 client->setPrintResponseBody(true);
 client->setContentTypeHeader("application/json");
 if (client != nullptr){
  if(!client->connected()){
    client->connect(host, httpsPort);
    switch (api){
      case READ:
        client->GET(url_read, host);
        result = client->getResponseBody();
        if(result != current_data){
          process_data(result);
          current_data = result;
        break;
      case WRITE:
        break;
      case POKESTATUS:
        client->GET(url_pokeStatus, host);
        result = client->getResponseBody();
        if(result.indexOf("poke") != -1){
          poke_Event();    
          // digitalWrite(2, HIGH)
        }
        break;
      case UNPOKE:
        client->GET(url_unpoke, host);
        result = client->getResponseBody();
        if(result.indexOf("Written on column D") != -1){
          ;//poke back here?    
          // digitalWrite(2, HIGH)
        }
        break;
      default:
        break;
      }
    }

    }
  }
  delete client;
  client = nullptr;
}

void pull_weather_data (int city){
  
  String location;
  if (city == 0){
    location = "Bangalore,in";
  }
  else if (city == 1){
    location = "San%20Jose,us";
  }

  HTTPClient http;
  WiFiClient client;
  http.begin(client,"http://api.openweathermap.org/data/2.5/weather?q=" + location + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2");
  int httpCode = http.GET(); // send the request

  if (httpCode > 0){
    String payload = http.getString(); //Get the request response payload
  
    DynamicJsonBuffer jsonBuffer(512);
  
    // Parse JSON object
    Serial.println(payload);
    JsonObject& root = jsonBuffer.parseObject(payload);
    if (!root.success()) {
    Serial.println(F("Parsing failed!"));
    return;
    }
    // JsonArray& list = root["list"];
    //which icon to use
    if(city == 0){
      blr_weather_icon = root["weather"][0]["icon"].as<String>();
      correct_icon_names(blr_weather_icon);
    }
    else if (city == 1){
      sjc_weather_icon = root["weather"][0]["icon"].as<String>();
      correct_icon_names(sjc_weather_icon);
    }
  }
}

void correct_icon_names(String& in){
  if (in == "03n") {
      in = "03d";
  }
  else if (in == "04n") {
      in = "04d";
  }
  else if (in == "09n") {
      in = "09d";
  }
  else if (in == "10n") {
      in = "10d";
  }
  else if (in == "11n") {
      in = "11d";
  }
  else if (in == "13n") {
      in = "13d";
  }
  else if (in == "50n") {
      in = "50d";
  }
}

// This next function will be called during decoding of the jpeg file to
// render each block to the TFT.  If you use a different TFT library
// you will need to adapt this function to suit.
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  // tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}

void get_image_from_SPIFFS(int x, int y, String image_id){
  // Get the width and height in pixels of the jpeg if you wish
  uint16_t w = 0, h = 0;
  String file_name = "/" + image_id + ".jpg";
  TJpgDec.getFsJpgSize(&w, &h, file_name); // Note name preceded with "/"
  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
  TJpgDec.drawFsJpg(x, y, file_name);
  delay(2000);
}


void jimmy_alert(){
  tft.drawLine(0,3*tft.height()/5+20, tft.width(), 3*tft.height()/5+20, ST77XX_WHITE);
  //title
  tft.setTextSize(1);
  tft.setCursor(tft.height()/5+10, 103);
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Jimmy?");
  tft.drawLine(tft.width()/2, 3*tft.height()/5+20, tft.width()/2, tft.height(), ST77XX_WHITE); //the great divide
  tft.setCursor(0,3*tft.height()/5+25);
  tft.println("BLR:");
  tft.setCursor(tft.width()/2+2, 3*tft.height()/5+25);
  tft.println("SJC:");
}

void jimmy_update(){
  get_image_from_SPIFFS(12, 3*tft.height()/5+35, blr_weather_icon);
  get_image_from_SPIFFS(tft.width()/2+14, 3*tft.height()/5+35, sjc_weather_icon);
}
// ***************************************************************************************
void listSPIFFS(void) {
  Serial.println(F("\r\nListing SPIFFS files:"));

  fs::Dir dir = SPIFFS.openDir("/"); // Root directory

  static const char line[] PROGMEM =  "=================================================";
  Serial.println(FPSTR(line));
  Serial.println(F("  File name                              Size"));
  Serial.println(FPSTR(line));

  while (dir.next()) {
    String fileName = dir.fileName();
    Serial.print(fileName);
    int spaces = 33 - fileName.length(); // Tabulate nicely
    if (spaces < 1) spaces = 1;
    while (spaces--) Serial.print(" ");

    fs::File f = dir.openFile("r");
    String fileSize = (String) f.size();
    spaces = 10 - fileSize.length(); // Tabulate nicely
    if (spaces < 1) spaces = 1;
    while (spaces--) Serial.print(" ");
    Serial.println(fileSize + " bytes");
  }

  Serial.println(FPSTR(line));
  Serial.println();
  delay(1000);
}

//GLOBAL Variables
int poke_button = 0; 
unsigned long lastConnectionTime = 0;  // last time you connected to the server, in milliseconds
const unsigned long owmInterval = 10 * 60 * 1000; // Interval between OWM requests (10 minutes)
unsigned long lastClockUpdate = 0; // Time of the last clock update
const unsigned long oneMin = 60 * 1000; // 1 minute in milliseconds
int data_loop = 5; // To fetch data periodically
bool first_time = true;


void setup() {
    // Initialise SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nInitialisation done.");
  //display setup
  pinMode(LED_BUILTIN, OUTPUT); //poke status led
  pinMode(16, INPUT_PULLDOWN_16); // declare push button as input
  tft.initR(INITR_BLACKTAB);
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(WHITE);
  tft.setCursor(0, 0);
  Serial.begin(115200);
  // Display static text
  tft.setCursor(0, 0);    //Horiz/Vertic
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Hi da, Connect to ConfigureWifi from your phone and setup Wifi :)");
  WiFiManager wifiManager;
  wifiManager.autoConnect("ConfigureWifi");
  //WIFI connection
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.setCursor(0, 0); 
  tft.print("Connecting to the internet, setting up widgets");
  delay(4000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.setCursor(0, 0);    //Horiz/Vertic
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Hello :)"); 
  delay(2000);
  
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(4);
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
  // listSPIFFS(); 
  //create widgets
  date_time_Widget(); //top widget for date and time
  message_Widget();
  jimmy_alert();
  poke_Widget();
  display_time();
  get_data(READ); 
  get_data(POKESTATUS);
  pull_weather_data(BLR);
  pull_weather_data(SJC);
  jimmy_update();

}


void loop(){
  poke_button = digitalRead(16);
  //check time once every_minute:
  if (millis() - lastClockUpdate >= oneMin) {
    lastClockUpdate = millis();
    display_time();
    get_data(READ); 
    get_data(POKESTATUS);
  }
  if(scroll){
    process_data(current_data);
  }
  if (poke_status){
    breathe_led(); //notification that a poke has occured
  }
  if (poke_button == HIGH) {
    poke_Event(true);
    get_data(UNPOKE);
    }
  //OWM requires 10mins between request intervals
  //check if 10mins has passed then conect again and pull
 if (millis() - lastConnectionTime >= owmInterval) {
    lastConnectionTime = millis();
    pull_weather_data(BLR);
    pull_weather_data(SJC);
    jimmy_update();
  }
}
