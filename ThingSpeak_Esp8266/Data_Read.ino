#include <SoftwareSerial.h>
#include <DHT11.h>

#define LEDPIN 1

SoftwareSerial Wifi =  SoftwareSerial(12,13);   //Wifi객체생성, esp8266 rx11, tx12(NANO), esp8266 rx12, tx13(UNO)
int Field_Index = 2;
boolean DEBUG = true;
String apiKey = "HC39Y2NQKXUV0HWW";     // ThingSpeak Apikey
String Channel_ID = "247820";           // ThingSpeak Channel ID
String ssid = "olleh_WiFi_889F";             // Wifi network SSID
String password ="0000007171";            // Wifi network password


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600); 
  Wifi.begin(9600);
  //pinMode(LEDPIN, OUTPUT);
  Serial.println("--Setup--");
  SetWifi(ssid, password, DEBUG);
  if (DEBUG)  
    Serial.println("Setup completed\n");
}

void loop() {
  if (false == ThingSpeakWrite(Field_Index, DEBUG)) { // Read values to thingspeak

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

boolean ThingSpeakWrite(int Field_Index, boolean DEBUG){
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
  
  String GetStr = GetDString(apiKey, Channel_ID, Field_Index);  
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
    if(Wifi.find("+IPD,5:")){
      String data=Wifi.readString();
      Serial.print("getdata = ");
      Serial.println(data);
      int num=(data[0]-48)*10;
      num+=(data[1]-48);
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
