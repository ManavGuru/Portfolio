#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_ST7735.h> 

#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include "DebugMacros.h"
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <WifiUDP.h>
#include <NTPClient.h>


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// extern const char* ssid;
// extern const char* password;
// extern const char* host;
// extern const char *GScriptId;

const char* ssid = "ath-sjc";
const char* password = "#2134@sjc6918";

const char* host = "script.google.com";
const char *GScriptId = "AKfycbwJhzysQyX0LskwxihYPwX0KDFrhbmbX6UXFfMbEqRBYOB9OrXQiKrg_R2uK8pR-6k";

extern  unsigned char  cloud[];
extern  unsigned char  thunder[];
extern  unsigned char  wind[];


//GScript APIs
String url_read = String("/macros/s/") + GScriptId + "/exec?read";
String url_write = String("/macros/s/") + GScriptId + "/exec?write";
const uint16_t httpsPort = 443;
String current_data = "";

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
    delay(20);
    digitalWrite(pin, LOW);
    delay(30);
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
        if (f == NULL) tft.print("  "); //rubout trailing chars
        delay(2);
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
        scroll = true;
        tft.setCursor(0, 40);    //Horiz/Vertic
        scrolltext(0,40,data);
      }
}

void get_data(){
 static bool flag = false; 
 client = new HTTPSRedirect(httpsPort);
 client->setInsecure();
 client->setPrintResponseBody(true);
 client->setContentTypeHeader("application/json");
 
 if (client != nullptr){
  if(!client->connected()){
    client->connect(host, httpsPort);
    client->GET(url_read, host);
    result = client->getResponseBody();
    if(result != current_data){
        process_data(result);
        current_data = result;
      }
    }
  }
  delete client;
  client = nullptr;
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
void poke_Event(){
    tft.fillRect(0, 2.5*tft.height()/5+1, tft.width(), tft.height()/10, ST77XX_ORANGE);
    //if incoming poke change to green
    tft.fillRect(0, 2.5*tft.height()/5+1, tft.width(), tft.height()/10, ST77XX_GREEN);
    tft.setCursor(tft.height()/5+2, tft.width()/5);
    tft.println("YOU HAVE BEEN POKED :P");
    //if outgoing change to sending poke animation 
}


void setup() {
  //display setup
  pinMode(LED_BUILTIN, OUTPUT); //poke status led
  tft.initR(INITR_BLACKTAB);
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(WHITE);
  tft.setCursor(0, 0);
  
  // Display static text
  tft.print("Hello :)"); 
  delay(2000);
  tft.setCursor(0, 0);    //Horiz/Vertic
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);

  //WIFI connection
  tft.print("Connecting to:");
  tft.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    tft.print(".");
    blink_led();
  }
  
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(true);
  tft.setCursor(0, 0);    //Horiz/Vertic
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(WiFi.localIP());
  tft.fillScreen(ST77XX_BLACK);
  //create widgets
  date_time_Widget(); //top widget for date and time
  message_Widget();
  jimmy_alert();
  poke_Widget();
}

int data_loop = 5; //to fetch data periodically 
bool poke_status = true;

void loop(){
  display_time();
  if(data_loop == 5){
    get_data();
    data_loop = 0;
    poke_Event();
    poke_status = !poke_status;
  }
  data_loop++;
  if(scroll)
    process_data(current_data);

  if (poke_status) {
    blink_led();
    digitalWrite(LED_BUILTIN, LOW);
  }
}