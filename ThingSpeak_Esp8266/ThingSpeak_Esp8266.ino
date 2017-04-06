/*  Updated 2017-03-27
    Created by Sangmin Park
    Project(Sensor Part)
    Sensor -> Arduino -> Esp8266 -> ThingSpeak
*/
#include <SoftwareSerial.h>
#include <DHT11.h>
SoftwareSerial Wifi =  SoftwareSerial(12, 13);  //Wifi객체생성, esp8266 rx11, tx12(NANO), esp8266 rx12, tx13(UNO)

#define DHTPIN A0             //온습도센서 아날로그 핀
#define GASPIN A1             //가스센서 아날로그 핀
#define DUSTPIN A2             //미세먼지센서 아날로그 핀
#define LEDPIN 3              //신호 알림 LED
#define DUST_LEDPIN 4              //미세먼지 ILED

DHT11 dht11(DHTPIN);         //온습도 센서 객체 생성!
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
boolean DEBUG = true;
String apiKey = "HC39Y2NQKXUV0HWW";     // ThingSpeak Apikey
String ssid = "sangmin24";             // Wifi network SSID
String password = "45701967";           // Wifi network password
float temp, humi, gas, dust, gasVoltage, gasDensity ,dustVoltage, dustDensity;
//================================================================================ setup
//            ||**Setting**||

void setup()
{
  Serial.begin(9600);
  Wifi.begin(9600);
  pinMode(LEDPIN, OUTPUT);
  pinMode(DUST_LEDPIN,OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  Serial.println("--Setup--");
  SetWifi(ssid, password, DEBUG);
  if (DEBUG)
    Serial.println("Setup completed");
  digitalWrite(LEDPIN, LOW);
}


// ====================================================================== loop
//             ||**Main Loop**||
void loop()
{
  // Read sensor values
  digitalWrite(DUST_LEDPIN,LOW); // power on the LED
  delayMicroseconds(samplingTime);
  dht11.read(humi, temp);
  gas =  analogRead(GASPIN);
  gasVoltage = gas * 5.0 / 1024.0;
  gasDensity = 0.17 * gasVoltage - 0.1;   //unit ppm
  dust = analogRead(DUSTPIN);
  dustVoltage = dust * 5.0 / 1024.0;
  dustDensity = 0.17 * dustVoltage - 0.1; //unit ug/m3
  dustDensity *= 1000;
  dustDensity = (dustDensity + 120)*(1 + (100 + dustDensity) / 100); //마이크로 그램으로 변환
  delayMicroseconds(deltaTime);
  digitalWrite(DUST_LEDPIN,HIGH); // turn the LED off
  delayMicroseconds(sleepTime);
  if (isnan(temp) || isnan(humi)) {
    if (DEBUG)
      Serial.println("Failed to read from DHT");
  } else if (isnan(gas)) {
    if (DEBUG)
      Serial.println("Failed to read from GAS");
  } else if (isnan(dust)) {
    if (DEBUG)
      Serial.println("Failed to read from DUST");
  } else {
    if (DEBUG) {
      Serial.println("Temp=" + String(temp) + "℃");
      Serial.println("Humidity=" + String(humi) + "%");
      Serial.println("Gas=" + String(gasDensity) + "ppm");
      Serial.println("Fine Dust=" + String(dustDensity) + "ug/m3");
    }
    if (false == ThingSpeakWrite(temp, humi, gasDensity, dustDensity , DEBUG)) {                                   // Write values to thingspeak
      
      return;
    }
  }
  // thingspeak needs 30 sec delay between updates,
  delay(1000);
}


//            || **Methods** ||

void SetWifi(String ssid, String pw, boolean DEBUG)
{
  Serial.println("--SetWifi--");
  digitalWrite(LEDPIN, HIGH);
  SendData("AT+RST\r\n", 2000, DEBUG); // reset module
  SendData("AT+CIOBAUD?\r\n", 2000, DEBUG); // check baudrate (redundant)
  SendData("AT+CWMODE=3\r\n", 1000, DEBUG); // configure as access point (working mode: AP+STA)
  SendData("AT+CWLAP\r\n", 3000, DEBUG); // list available access points
  SendData("AT+CWJAP=\"" + ssid + "\",\"" + password + "\"", 5000, DEBUG); // join the access point
  SendData("AT+CIFSR\r\n", 1000, DEBUG); // get ip address
  SendData("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
  SendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG); // turn on server on port 80
  digitalWrite(LEDPIN, LOW);
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

String Get4DString(String ApiKey, float Field1 = 0, float Field2 = 0, float Field3 = 0, float Field4 = 0) {
  String temp_str = "GET /update?api_key=";   // prepare GET string
  temp_str += ApiKey;
  temp_str += "&field1=";
  temp_str += String(Field1);
  temp_str += "&field2=";
  temp_str += String(Field2);
  temp_str += "&field3=";
  temp_str += String(Field3);
  temp_str += "&field4=";
  temp_str += String(Field4);
  temp_str += "\r\n\r\n";
  return temp_str;
}

//========================================================================

boolean ThingSpeakWrite(float value1, float value2, float value3, float value4, boolean DEBUG) {
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

  String GetStr = Get4DString(apiKey, value1, value2, value3, value4);      //data temp, humi, gas, fine dust(4개)
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
    digitalWrite(LEDPIN, HIGH);
    delay(200);
    digitalWrite(LEDPIN, LOW);
    Wifi.print(GetStr);
    if (DEBUG) {
      Serial.println("Success Send Data");
      Serial.print("Contents : ");
      Serial.print(GetStr);
    }
  }
  else
  {
    Wifi.println("AT+CIPCLOSE");
    // alert user
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
      delay(200);
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
    if (DEBUG) {
      Serial.println("Failed Send Data");
      Serial.println("AT+CIPCLOSE");
    }
    return false;
  }
  return true;
}
