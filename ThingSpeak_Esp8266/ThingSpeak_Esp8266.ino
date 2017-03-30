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
boolean DEBUG = false;
String apiKey = "HC39Y2NQKXUV0HWW";     // ThingSpeak Apikey
String ssid = "sangmin24";             // Wifi network SSID
String password = "45701967";           // Wifi network password
float temp, humi, gas, dust;
//================================================================================ setup
//            ||**Setting**||

void setup()
{
  Serial.begin(9600);
  Wifi.begin(9600);
  pinMode(LEDPIN, OUTPUT);
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
  dht11.read(humi, temp);
  gas =  analogRead(GASPIN) / 100.0;
  dust = analogRead(DUSTPIN);
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
      Serial.println("Gas=" + String(gas) + "ppm'");
      Serial.println("Fine Dust=" + String(dust) + "ppm");
    }
    if (false == ThingSpeakWrite(temp, humi, gas, dust , DEBUG)) {                                   // Write values to thingspeak
      
      return;
    }
  }
  // thingspeak needs 15 sec delay between updates,
  delay(60000);
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

String Get4DString(String ApiKey, int Field1 = 0, int Field2 = 0, int Field3 = 0, int Field4 = 0) {
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
  Serial.print(GetStr);
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
