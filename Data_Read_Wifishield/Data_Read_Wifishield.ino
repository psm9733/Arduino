
/*  Created 2017-05-22

    Created by Sangmin Park

    Project(Sensor Part)

    Arduino -> Esp8266 -> ThingSpeak -> Esp8266 ->Arduino -> Sensor

    reference library Thread -> https://github.com/ivanseidel/ArduinoThread
*/

/* Licence

The MIT License (MIT)

Copyright (c) 2015 Ivan Seidel

Permission is hereby granted, free of charge, to any person obtaining a copy

of this software and associated documentation files (the "Software"), to deal

in the Software without restriction, including without limitation the rights

to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

copies of the Software, and to permit persons to whom the Software is

furnished to do so, subject to the following conditions:

 

The above copyright notice and this permission notice shall be included in all

copies or substantial portions of the Software.

 

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE.

 */
#include <SPI.h>
#include <WiFi.h>
#include <Thread.h>
#include <ThreadController.h>
#define ALERT_LEDPIN 6
#define FAN_PIN 9
#define FANENA_PIN 10
#define LED_PIN 11
#define led_delay 50
#define wifi_delay 200
#define Thread_interval1 100
#define Thread_interval2 100
#define updateThingSpeakInterval 0
// Local Network Settings
char ssid[] = "team9";  // your network SSID (name)
char pass[] = "6c700644";    // your network password
char thingSpeakAddress[] = "api.thingspeak.com";
int Field_Index = 1;
int keyIndex = 0;             // your network key Index number (needed only for WEP)
int status = WL_IDLE_STATUS;
int semaphore = 0;

String apiKey1 = "HC39Y2NQKXUV0HWW";      // ThingSpeak Led_ctrl Apikey
String apiKey2 = "I07Y6U9TP4JMYL3P";      // ThingSpeak Fan_ctrl Apikey
String Channel_ID1 = "257666";             // ThingSpeak Channel ID
String Channel_ID2 = "257667";
boolean DEBUG = true;
long lastConnectionTime = 0;
boolean lastConnected = false;

WiFiServer server(80);
WiFiClient client;
ThreadController controller = ThreadController();

void sendtoThingSpeak(String ApiKey, String Channel_ID, int Field_Index = 1) {
  if (client.connect(thingSpeakAddress, 80)) {
    String SendData = "GET /channels/";   // prepare GET string
    SendData += String(Channel_ID);
    SendData += "/fields/";
    SendData += String(Field_Index);
    SendData += "/last.txt?key=";
    SendData += ApiKey;
    SendData += "\r\n\r\n";
    client.print(SendData);
    lastConnectionTime = millis();

    if (client.connected()) {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      digitalWrite(ALERT_LEDPIN, HIGH);
      delay(led_delay);
      digitalWrite(ALERT_LEDPIN, LOW);
    }else{
      digitalWrite(ALERT_LEDPIN, HIGH);
      delay(led_delay);
      digitalWrite(ALERT_LEDPIN, LOW);
      delay(led_delay);
      digitalWrite(ALERT_LEDPIN, HIGH);
      delay(led_delay);
      digitalWrite(ALERT_LEDPIN, LOW);
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
boolean getdata(int Field_Index, boolean DEBUG, String apiKey, String Channel_ID, int Pin){
  if (client.available()) {
    char c = client.read();
    if(DEBUG == true){
      if(Pin == 10){
        Serial.println("Fan");
      }else if(Pin == 11){
        Serial.println("LED");
      }
      if(c == 49){
        Serial.println("ON");
      }else{
        Serial.println("OFF");
      }
    }
  }
  
  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected) {
    Serial.println("...disconnected");
    Serial.println();
    client.flush();
    client.stop();
  }
    
  // Update ThingSpeak
  if (!client.connected() && (millis() - lastConnectionTime > updateThingSpeakInterval)) {
    sendtoThingSpeak(apiKey, Channel_ID, Field_Index);
    return true;
  }
  lastConnected = client.connected();
  delay(1000);
}
// callback for LedThread
void LedCallback(){
  if(semaphore == 1){
    return;
  }
  if(DEBUG)
    Serial.println("Led Thread Operating");
  if (false == getdata(Field_Index, DEBUG, apiKey1, Channel_ID1, LED_PIN)) { // Read values to thingspeak
    if(DEBUG == true){
      Serial.println("getdata Error");
    }
  }
  semaphore = 1;
}

void FanCallback(){
  if(semaphore == 0){
    return;
  }
  if(DEBUG)
    Serial.println("Fan Thread Operating");
  if (false == getdata(Field_Index, DEBUG, apiKey2, Channel_ID2, FANENA_PIN)) { // Read values to thingspeak
    if(DEBUG == true){
      Serial.println("getdata Error");
    }
  }
  delay(wifi_delay);
  semaphore = 0;
}

void setup() {
  // Start Serial for debugging on the Serial Monitor
  Serial.begin(9600);
  Thread LedThread = Thread();
  Thread FanThread = Thread();
  LedThread.onRun(LedCallback);
  FanThread.onRun(FanCallback);
  LedThread.setInterval(Thread_interval1);
  FanThread.setInterval(Thread_interval2);
  controller.add(&LedThread);
  controller.add(&FanThread);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }
  // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
}

void loop() {
  controller.run();
}

