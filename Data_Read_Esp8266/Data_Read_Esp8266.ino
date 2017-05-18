
/*  Updated 2017-05-19

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
#include <SoftwareSerial.h>
#include <DHT11.h>
#include <Thread.h>
#include <ThreadController.h>

#define ALERT_LEDPIN 6
#define FANPIN 9
#define FANENA_PIN 10
#define LEDPIN 11
#define led_delay 50

SoftwareSerial Wifi =  SoftwareSerial(12, 13);  //Wifi객체생성, esp8266 rx11, tx12(NANO), esp8266 rx12, tx13(UNO)

int Field_Index = 1;
boolean DEBUG = true;
String apiKey1 = "HC39Y2NQKXUV0HWW";      // ThingSpeak Led_ctrl Apikey
String apiKey2 = "I07Y6U9TP4JMYL3P";      // ThingSpeak Fan_ctrl Apikey
String Channel_ID1 = "257666";             // ThingSpeak Channel ID
String Channel_ID2 = "257667";
String ssid = "team9";                    // Wifi network SSID
String password = "6c700644";             // Wifi network password
ThreadController controll = ThreadController();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wifi.begin(9600);
  pinMode(ALERT_LEDPIN, OUTPUT);
  pinMode(FANPIN, OUTPUT);
  pinMode(FANENA_PIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  digitalWrite(FANENA_PIN, HIGH);
  Serial.println("--Setup--");
  pinMode(FANPIN, OUTPUT);
  digitalWrite(ALERT_LEDPIN, HIGH);
  SetWifi(ssid, password, DEBUG);
  //Thread Create
  Thread LedThread = Thread();
  Thread FanThread = Thread();
  LedThread.onRun(LedCallback);
  FanThread.onRun(FanCallback);
  LedThread.setInterval(0);
  FanThread.setInterval(0);
  controll.add(&LedThread);
  controll.add(&FanThread);
  digitalWrite(ALERT_LEDPIN, LOW);
  digitalWrite(LEDPIN, LOW);
  digitalWrite(FANENA_PIN, LOW);
  if (DEBUG)
    Serial.println("Setup completed\n");
}

void loop() {
  controll.run();
}
//========================================================================
String SendData(String command, const int timeout, boolean DEBUG) {
  String response = "";
  Wifi.print(command); // send the read character to the esp8266
  long int time = millis();
  while ( (time + timeout) > millis())
  {
    while (Wifi.available()) {
      // The esp has data so display its output to the serial window
      char c = Wifi.read(); // read the next character.
      response += c;
    }
  }
  if (DEBUG)
  {
    Serial.print(response);
  }
  return response;
}

String GetDString(String ApiKey, String Channel_ID, int Field_Index = 0) {
  String temp_str = "GET /channels/";   // prepare GET string
  temp_str += String(Channel_ID);
  temp_str += "/fields/";
  temp_str += String(Field_Index);
  temp_str += "/last.txt?key=";
  temp_str += ApiKey;
  temp_str += "\r\n\r\n";
  return temp_str;
}

//========================================================================

void SetWifi(String ssid, String pw, boolean DEBUG)
{
  Serial.println("--SetWifi--");
  SendData("AT+RST\r\n", 2000, DEBUG); // reset module
  SendData("AT+CIOBAUD?\r\n", 2000, DEBUG); // check baudrate (redundant)
  SendData("AT+CWMODE=3\r\n", 1000, DEBUG); // configure as access point (working mode: AP+STA)
  SendData("AT+CWLAP\r\n", 3000, DEBUG); // list available access points
  SendData("AT+CWJAP=\"" + ssid + "\",\"" + password + "\"", 5000, DEBUG); // join the access point
  SendData("AT+CIFSR\r\n", 1000, DEBUG); // get ip address
  SendData("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
  SendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG); // turn on server on port 80
}

boolean ThingSpeakWrite(int Field_Index, boolean DEBUG, String apikey, String Channel_ID, int Pin) {
  String cmd = "AT+CIPSTART=\"TCP\",\"";                  // TCP connection
  cmd += "184.106.153.149";                               // api.thingspeak.com
  cmd += "\",80";
  Wifi.println(cmd);
  if (DEBUG)
    Serial.println(cmd);
  if (Wifi.find("Error"))
  {
    if (DEBUG)
      Serial.println("AT+CIPSTART error");
    return false;
  }
  String GetStr = GetDString(apikey, Channel_ID, Field_Index);
  // Send data length
  cmd = "AT+CIPSEND=";
  cmd += String(GetStr.length());
  Wifi.println(cmd);
  if (DEBUG)
    Serial.println(cmd);
  delay(100);
  if (Wifi.find(">"))
  {
    // Send Data String
    Wifi.print(GetStr);
    if (DEBUG) {
      Serial.println("Success Send Data");
      Serial.print("Contents : ");
      Serial.print(GetStr);
    }
    digitalWrite(ALERT_LEDPIN, HIGH);
    delay(led_delay);
    digitalWrite(ALERT_LEDPIN, LOW);
    if (Wifi.find("+IPD,")) {                     //Wifi.find("+IPD,") => "tempnumber:data"
      String data = Wifi.readString();
      if (DEBUG) {
        Serial.print("getdata = ");
        Serial.println(data[2]);                //data[2] = data
        Serial.println("\n"); 
      }
      if (data[2] == 49)                         //49 = "1"
        digitalWrite(Pin, HIGH);
      else
        digitalWrite(Pin, LOW);
    }
  }
  else
  {
    Wifi.println("AT+CIPCLOSE");
    // alert user
    digitalWrite(ALERT_LEDPIN, HIGH);
    delay(led_delay);
    digitalWrite(ALERT_LEDPIN, LOW);
    delay(led_delay);
    digitalWrite(ALERT_LEDPIN, HIGH);
    delay(led_delay);
    digitalWrite(ALERT_LEDPIN, LOW);
    if (DEBUG) {
      Serial.println("Failed Read Data");
      Serial.println("AT+CIPCLOSE\n");
    }
    delay(500);
    return false;
  }
  delay(500);
  return true;
}
// callback for LedThread
void LedCallback(){
  if(DEBUG)
    Serial.println("Led Thread Operating");
  if (false == ThingSpeakWrite(Field_Index, DEBUG, apiKey1, Channel_ID1, LEDPIN)) { // Read values to thingspeak
  }
}
// callback for FanThread
void FanCallback(){
  if(DEBUG)
    Serial.println("Fan Thread Operating");
  if (false == ThingSpeakWrite(Field_Index, DEBUG, apiKey2, Channel_ID2, FANENA_PIN)) { // Read values to thingspeak
  }
}
