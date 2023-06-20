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

//host server
const char* host = "script.google.com";
const char *GScriptId = "AKfycbxKAaekPVHQYyC3ZawrfBqyl6isiUH_KxwzppJqQW5GxNZ9oK4CXUfFVUwaeS3xgVY";

//response server
String BlrWeather = "https://api.openweathermap.org/data/2.5/forecast?q=Bangalore,in&APPID=0c40676db9bcb51086ea89239551ca2b&units=metric&cnt=2"; 
String SjcWeather = "https://api.openweathermap.org/data/2.5/forecast?q=San%20Jose,us&APPID=0c40676db9bcb51086ea89239551ca2b&units=metric&cnt=2";

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
void breathe_led(int pin= LED_BUILTIN){
  float smoothness_pts = 500;//larger=slower change in brightness  
  float gamma = 0.14; // affects the width of peak (more or less darkness)
  float beta = 0.5; // shifts the gaussian to be symmetric

  for (int ii=0;ii<smoothness_pts;ii++){
    float pwm_val = 255.0*(exp(-(pow(((ii/smoothness_pts)-beta)/gamma,2.0))/2.0));
    analogWrite(LED_BUILTIN,int(pwm_val));
    delay(5);
  }
}

String remove_Spaces(String inputString){
  int i, j;
  for(j = -1, i = 0; inputString[i] != '\0'; i++) {
    if((inputString[i]>='a' && inputString[i]<='z') || 
    (inputString[i]>='A' && inputString[i]<='Z')) {
        inputString[++j] = inputString[i];
      }
    }
    inputString[j] = '\0';

    return inputString;
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
        scroll = true;
        tft.setCursor(0, 40);    //Horiz/Vertic
        scrolltext(0,40,data);
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

int poke_button = 0; 
void setup() {
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

  //create widgets
  date_time_Widget(); //top widget for date and time
  message_Widget();
  jimmy_alert();
  poke_Widget();
}

int data_loop = 5; //to fetch data periodically 

void loop(){
  poke_button = digitalRead(16);
  display_time();
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
  if(data_loop == 15){
    get_data(READ); 
    get_data(POKESTATUS);
    data_loop = 0;
    //if pStatus == Poke
    //Trigger poke event
    //set Unpoke
  }
  data_loop++;
}
