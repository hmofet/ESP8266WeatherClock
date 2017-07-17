/*
 * 
 * WeatherClock with Web Server and SSD1306 OLED display
 * Compiled and heavily modified from multiple sources (attributions given)
 * by Arin Bakht (www.github.com/hmofet)
 * Released under BSD licence
 * 
 * The clock is hardcoded for Eastern Standard Time (defaults to Daylight Saving Time, switchable
 * at run-time by navigating to the webserver)
 * 
 * Designed to be run with an Adafruit-compatible SSD1306 OLED 128x64 display connected through I2C
 * Designed to be run on a NodeMCU devkit, could be adapted for other ESP8266-based boards
 * If the ESP8266 cannot connect to any wifi networks that it knows about, it will go into AP mode
 * and put up a configuration web page at 192.168.4.1 (using wifiManager library). You can connect
 * to this ad-hoc network and configure the wifi settings on a phone or other wifi device.
 * 
 * Weather data pulled from OpenWeatherMap API. Please use your own API key (registration for basic account is free)
 * 
 * 
 */

//Below is the copyright notice for SSD1306 library example provided by Adafruit Industries

/*********************************************************************
This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/

//Above ends the copyright notice for SSD1306 library example provided by Adafruit Industries

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Time.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>

#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <ESP8266HTTPClient.h>


#include <DNSServer.h>  //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>  

#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

#define CELSIUS "'C"
#define KPASCALS "kPa"
#define MILLIS_IN_SECOND 1000
#define SECONDS_IN_DAY 86400
#define SECONDS_IN_HOUR 3600
#define SECONDS_IN_MINUTE 60
#define SECONDS 1;

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif


//define struct used to contain forecast data pulled from OpenWeatherMap API
typedef struct {
  const char* desc;
  const char* temp;
  const char* tempMin;
  const char* tempMax;
  const char* pressure; 
} Forecast;

int tzOffset = -4;
int hits = 0;
time_t currentTime;
ESP8266WebServer server(80);
Forecast forecast;

void setup()   {        
  
  //begin serial connection        
  Serial.begin(115200);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  //display helpful message to tell user how to configure wifi for first-run
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Connect to 192.168.4.1");
  display.println("in order to configure WiFi");
  display.display();



 // WiFi.begin("junk","junk"); use this gibberish password to clear AP memory

  WiFiManager wifiManager;
  if(wifiManager.autoConnect("WeatherClock")){
    display.clearDisplay();
    display.setCursor(0,0);
    display.println( "Wi-Fi Connected");
    display.println(WiFi.localIP());
    display.display();
    delay(2000);
    display.clearDisplay();
  }

  if(!wifiManager.autoConnect("WeatherClock")){
    display.clearDisplay();
    display.println("Retrying Wi-Fi...");
    display.display();
    delay(2000);
    //ESP.reset(); //this really messes up the ESP
  }

  //webUnixTime function requires wificlient to be passed to it
  static WiFiClient client;
  currentTime = webUnixTime(client);
  
  setTime(currentTime);
                   
   //begin listening on DNS
  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  //what function to call when navigating to root path
  server.on("/", handleRoot);

  //and on error
  server.onNotFound(handleNotFound);

  //begin server
  server.begin();
  Serial.println("HTTP server started");

}


void loop() {
      
      //Arduino ESP8266 GET request code courtesy BasicHttpClient example from ESP8266 Arduino library

      
        HTTPClient http;
        static WiFiClient client;

        http.begin("http://api.openweathermap.org/data/2.5/weather?q=Toronto&units=metric&APPID=[INSERT_API_KEY_HERE]"); //HTTP
        
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if(httpCode > 0) {
            // HTTP header has been send and Server response header has been handled
        
            // file found at server
            if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();

                //close session since we got what we needed
                http.end();

                //TODO: change to static buffer to save memory
                DynamicJsonBuffer jsonBuffer;
                JsonObject& root = jsonBuffer.parseObject(payload);

                //parse out JSON
                forecast.desc = root["weather"][0]["description"];
                forecast.temp = root["main"]["temp"];
                forecast.pressure = root["main"]["pressure"];
                forecast.tempMin = root["main"]["temp_min"];
                forecast.tempMax = root["main"]["temp_max"];

      
                //loop lots of times before hitting server again so don't hit the limit for free OpenWeatherMap accounts
                for(int i = 0; i < 100; i++){

                  //print various screens of the forecast  
                  printForecast("Today's forecast is", forecast.desc, "", true);
                  printForecast("The current temp is", forecast.temp, CELSIUS, false);
                  printForecast("Today's high is", forecast.tempMax, CELSIUS, false);
                  printForecast("Today's low is", forecast.tempMin, CELSIUS, false);
                  printForecast("The air pressure is", forecast.pressure, KPASCALS, false);
                  
                  //print out stats
                  printSystemStats();                   

                }
          
                
            }
            
        } else {
          //if there was an error connecting via HTTP
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.setCursor(0,0);
          display.println("ERROR");
          display.display();
          delay(5000);
          display.clearDisplay();
        }

        http.end();

  
}

void printForecast(String message, const char* value, String units, bool forecast){

  //occasionally call web server otherwise it's non-responsive
  server.handleClient();
  
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(WHITE);
      display.setCursor(0,0);
      display.println("The current time is:");
      display.setTextSize(2);
      display.println(calculateTime());
      display.setTextSize(1);
      //print an extra empty line so time is separated from conditions. In text forecast, not enough space for this
      if(!forecast)
        display.println();
      display.println(message + ":");
      display.setTextSize(2);
      display.println(value + units);
      display.display();
 
 //occasionally call web server otherwise it's non-responsive     
 server.handleClient();

      //scroll text if text forecast
      if (forecast){
        delay(1000);
        display.startscrollright(0x0C, 0x0F); //scroll from 12th to 16th rows (in hex, 16 rows max) 
        delay(8000);
        display.stopscroll();
      } else {
        delay(5000);
      }
 
 //occasionally call web server otherwise it's non-responsive     
 server.handleClient();    

}

//print time nicely
String calculateTime(){

  String time = "";
  short hr = hourFormat12();

  time = String(hr) + ":";

  if (minute() < 10){
    time += "0" + String(minute());
  } else {
    time += String(minute());
  }

  time += ":";  
  
  if (second() < 10){
    time += "0" + String(second());
  } else {
    time += String(second());
  }

  isAM() ? time += "AM" : time += "PM";

  
  return time;

}

//seconds to days/hrs/minutes/seconds code courtesy user Przemek, stackoverflow,
//https://stackoverflow.com/questions/2419562/convert-seconds-to-days-minutes-and-seconds
String uptime(){
     
  unsigned long input_millis = millis() / MILLIS_IN_SECOND;

  unsigned long days = input_millis / SECONDS_IN_DAY;
  unsigned long hours = (input_millis % SECONDS_IN_DAY) / SECONDS_IN_HOUR;
  unsigned long minutes = ((input_millis % SECONDS_IN_DAY) % SECONDS_IN_HOUR) / SECONDS_IN_MINUTE;
  unsigned long seconds = (((input_millis % SECONDS_IN_DAY) % SECONDS_IN_HOUR) % SECONDS_IN_MINUTE) / SECONDS;

  String uptime = "Uptime: " + String(days) + "d " + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s";
  
  return uptime;
 
}

//print out some basic stats
void printSystemStats(){
    int t_delay = 1500;
  
    //blank screen temporarilyprevent burnin on OLED screen
    display.clearDisplay();
    display.println();
    display.display();
    delay(750);
    
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,32);
    display.println(String(hits) + " hits on webpage");
    display.display();
    delay(t_delay);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,32);
    display.println(uptime());
    display.display();
    delay(t_delay);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,32);
    display.println( "Wi-Fi Connected");
    display.println(WiFi.localIP());
    display.display();
    delay(t_delay);
}

//code to run when client navigates to root path on server
void handleRoot() {
  
  //if GET param is not set, don't change time zone offset,
  //otherwise will reset time to UTC since tzOffset is null
  if(server.arg("tz") != NULL){
    tzOffset = server.arg("tz").toInt();
    static WiFiClient client;
    currentTime = webUnixTime(client);
    setTime(currentTime);
  }
  

  //increment hit counter
  ++hits;

  
  //generate HTTP response
  String contents = "<!DOCTYPE html>\n";
  contents += "<html>\n";
  contents +="<head>\n";
  contents += "<meta charset=\"utf-8\">\n";
  contents += "<title> NodeMCU </title>";
  contents += "</head>\n";
  contents += "<body>\n";
  contents += "<header><h1>NodeMCU Web Forecast</header>\n";
  contents += "<section>\n";
  contents += "The forecast is: ";
  contents += forecast.desc;
  contents += "<br>\n";
  contents += "The temperature is currently: ";
  contents += forecast.temp;
  contents += "°C<br>\n";
  contents += "The daily maximum is: ";
  contents += forecast.tempMax;
  contents += "°C<br>\n";
  contents += "The daily minimum is: ";
  contents += forecast.tempMin;
  contents += "°C<br>\n";
  contents += "The air pressure is: ";
  contents += forecast.pressure;
  contents += "kPa<br><br>\n";
  contents += "Time zone offset:";
  contents += "<form method=\"GET\">\n";
  contents += "<select name=\"tz\">\n";
  contents += "<option value=\"-4\">DST</option>";
  contents += "<option value=\"-5\">EST</option>";
  contents += "<input type=\"submit\" value=\"Submit\">";
  contents += "</select>\n";
  contents += "</form><br>\n";
  contents += "</section>\n";
  contents += "<footer>Current time: ";
  contents += calculateTime();
  contents += " ";
  contents += uptime();
  contents += "<br>";
  contents += String(hits);
  contents += " visits to the page.";
  contents += "</footer>\n";
  contents += "</body>\n";
  contents += "</html>";

  //send out response
  server.send(200, "text/html", contents);
}

//404 page
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}


//courtesy Francesco Potorti, Arduino Playground, http://playground.arduino.cc//Code/Webclient, GPLv3
//this function retrieves time from any webserver by sending an HTTP 1.1 request, as according to the standard,
//the server must respond with the time in the headers
//function then parses the time from human readable form (in UTC) to epoch time

unsigned long webUnixTime (Client &client)
{
  unsigned long time = 0;

  // Just choose any reasonably busy web server, the load is really low
  if (client.connect("google.ca", 80))
    {
      // Make an HTTP 1.1 request which is missing a Host: header
      // compliant servers are required to answer with an error that includes
      // a Date: header.
      client.print(F("GET / HTTP/1.1 \r\n\r\n"));
      char buf[5];      // temporary buffer for characters
      client.setTimeout(5000);
      if (client.find((char *)"\r\nDate: ") // look for Date: header
    && client.readBytes(buf, 5) == 5) // discard
  {
    unsigned day = client.parseInt();    // day
    client.readBytes(buf, 1);    // discard
    client.readBytes(buf, 3);    // month
    int year = client.parseInt();    // year
    byte hour = client.parseInt();   // hour
    byte minute = client.parseInt(); // minute
    byte second = client.parseInt(); // second
    int daysInPrevMonths;
    switch (buf[0])
      {
      case 'F': daysInPrevMonths =  31; break; // Feb
      case 'S': daysInPrevMonths = 243; break; // Sep
      case 'O': daysInPrevMonths = 273; break; // Oct
      case 'N': daysInPrevMonths = 304; break; // Nov
      case 'D': daysInPrevMonths = 334; break; // Dec
      default:
        if (buf[0] == 'J' && buf[1] == 'a')
    daysInPrevMonths = 0;   // Jan
        else if (buf[0] == 'A' && buf[1] == 'p')
    daysInPrevMonths = 90;    // Apr
        else switch (buf[2])
         {
         case 'r': daysInPrevMonths =  59; break; // Mar
         case 'y': daysInPrevMonths = 120; break; // May
         case 'n': daysInPrevMonths = 151; break; // Jun
         case 'l': daysInPrevMonths = 181; break; // Jul
         default: // add a default label here to avoid compiler warning
         case 'g': daysInPrevMonths = 212; break; // Aug
         }
      }

    // This code will not work after February 2100
    // because it does not account for 2100 not being a leap year and because
    // we use the day variable as accumulator, which would overflow in 2149
    day += (year - 1970) * 365; // days from 1970 to the whole past year
    day += (year - 1969) >> 2;  // plus one day per leap year 
    day += daysInPrevMonths;  // plus days for previous months this year
    if (daysInPrevMonths >= 59  // if we are past February
        && ((year & 3) == 0)) // and this is a leap year
      day += 1;     // add one day
    // Remove today, add hours, minutes and seconds this month
    time = (((day-1ul) * 24 + hour) * 60 + minute) * 60 + second;
  }
    }
  delay(10);
  client.flush();
  client.stop();

  //convert from UTC to EST/EDT
  if(tzOffset == -4){
    time -= 14400;
  } else {
    time -= 18000;
  }

  return time;
}

