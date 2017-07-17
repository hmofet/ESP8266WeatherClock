WeatherClock

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
