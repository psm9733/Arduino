

/*  Updated 2017-05-01
    Created by Sangmin Park
    Project(Sensor Part)
    Arduino -> Esp8266 -> ThingSpeak -> Esp8266 ->Arduino -> Sensor
*/
#include <SoftwareSerial.h>
#include <DHT11.h>

#define ALERT_LEDPIN 6
#define FANPIN 9
#define FANENA_PIN 10
#define LEDPIN 11
SoftwareSerial Wifi =  SoftwareSerial(12,13);   //Wifi객체생성, esp8266 rx11, tx12(NANO), esp8266 rx12, tx13(UNO)
int Field_Index = 1;
boolean DEBUG = true;
String apiKey1 = "HC39Y2NQKXUV0HWW";      // ThingSpeak Led_ctrl Apikey
String apiKey2 = "I07Y6U9TP4JMYL3P";      // ThingSpeak Fan_ctrl Apikey
String Channel_ID1 = "257666";             // ThingSpeak Channel ID
String Channel_ID2 = "257667";
String ssid = "team9";                    // Wifi network SSID
String password ="6c700644";              // Wifi network password


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); 
  Wifi.begin(9600);
  
  pinMode(ALERT_LEDPIN, OUTPUT);
  pinMode(FANPIN, OUTPUT);
  pinMode(FANENA_PIN, OUTPUT);
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(FANENA_PIN, HIGH);
  Serial.println("--Setup--");
  SetWifi(ssid, password, DEBUG);
  if (DEBUG)  
    Serial.println("Setup completed\n");
}

void loop() {
  if (false == ThingSpeakWrite(Field_Index, DEBUG, apiKey1, Channel_ID1, LEDPIN)) { // Read values to thingspeak

  }

  if (false == ThingSpeakWrite(Field_Index, DEBUG, apiKey2, Channel_ID2, FANPIN)) { // Read values to thingspeak

  }
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

String GetDString(String ApiKey, String Channel_ID, int Field_Index = 0){
  String temp_str = "GET /channels/";   // prepare GET string
  temp_str += String(Channel_ID);
  temp_str +="/fields/";
  temp_str += String(Field_Index);
  temp_str +="/last.txt?key=";
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
  SendData("AT+CWJAP=\""+ssid+"\",\""+password+"\"", 5000, DEBUG); // join the access point
  SendData("AT+CIFSR\r\n", 1000, DEBUG); // get ip address
  SendData("AT+CIPMUX=1\r\n", 1000, DEBUG); // configure for multiple connections
  SendData("AT+CIPSERVER=1,80\r\n", 1000, DEBUG); // turn on server on port 80
}

boolean ThingSpeakWrite(int Field_Index, boolean DEBUG, String apikey, String Channel_ID,int Pin){
  String cmd = "AT+CIPSTART=\"TCP\",\"";                  // TCP connection
  cmd += "184.106.153.149";                               // api.thingspeak.com
  cmd += "\",80";
  Wifi.println(cmd);
  if (DEBUG) 
    Serial.println(cmd);
  if(Wifi.find("Error"))
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
  if(Wifi.find(">"))
  {
    // Send Data String
    Wifi.print(GetStr);
    if (DEBUG){
      Serial.println("Success Send Data");
      Serial.print("Contents : ");
      Serial.print(GetStr); 
      //digitalWrite(LEDPIN, HIGH);
      //delay(200);
      //digitalWrite(LEDPIN, LOW);
    }
    if(Wifi.find("+IPD,")){                       //Wifi.find("+IPD,") => "tempnumber:data"
      String data=Wifi.readString();
      if (DEBUG){
        Serial.print("getdata = ");
        Serial.println(data[2]);                //data[2] = data
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
      /*
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
      delay(200);
      digitalWrite(LEDPIN, HIGH);
      delay(200);
      digitalWrite(LEDPIN, LOW);
      */
    if (DEBUG){
      Serial.println("Failed Read Data");
      Serial.println("AT+CIPCLOSE\n");

    }
    return false;
  }
  return true;
} 

