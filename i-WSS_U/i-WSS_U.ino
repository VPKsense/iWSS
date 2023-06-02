#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPLiwzn1ooT"
#define BLYNK_DEVICE_NAME "iWSS U"
#define BLYNK_AUTH_TOKEN "###"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "###";
char pass[] = "###";

//////V pins Define//////
#define Sitoutp V1
#define Floodp V2
#define RSSIp V3
////////////////////////

///Support variables/////
int pstat=0; 
int dat;
long rssi;
////////////////////////

//BlynkTimer timer;

BLYNK_WRITE(Floodp)
{
  dat=param.asInt();
  if(dat)
  digitalWrite(D3,LOW);
  else
  digitalWrite(D3,HIGH);
}

BLYNK_WRITE(Sitoutp)
{
  dat=param.asInt();
  if(dat)
  digitalWrite(D5,LOW);
  else
  digitalWrite(D5,HIGH);
}

BLYNK_WRITE(RSSIp)
{
  rssi = WiFi.RSSI();
  Blynk.virtualWrite(RSSIp,rssi);
}

BLYNK_CONNECTED()
{ if(!pstat)//for first connect after power on
  {
  Blynk.syncVirtual(Sitoutp,Floodp);
  pstat=1;
  digitalWrite(D4,HIGH);
  digitalWrite(D0,HIGH);
  }
  else
  {
    Blynk.syncVirtual(Sitoutp,Floodp);
    digitalWrite(D4,HIGH);
    digitalWrite(D0,HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v1.0.0 for i-WSS(U)-");
  Serial.println("Booting up...");
  pinMode(D4,OUTPUT);//Noconnection LED
  pinMode(D0,OUTPUT);
  OTA();
  Blynk.connectWiFi(ssid, pass);
  Blynk.config(auth/*IPAddress(68,183,87,221),8080*/);
  Blynk.connect(5);
  //timer.setInterval(30*1000, {Name});//30 seconds
  pinMode(D3,OUTPUT);//Flood light
  digitalWrite(D3,HIGH);
  pinMode(D5,OUTPUT);//Sitout light
  digitalWrite(D5,HIGH);
  delay(3000);
}

void loop()
{
  //timer.run();
  Blynk.run();
  ArduinoOTA.handle();
  if (!Blynk.connected()) 
  {  
    digitalWrite(D4,LOW);
    digitalWrite(D0,LOW);
  }   
}




void OTA()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) 
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setHostname("i-WSS-U");
  ArduinoOTA.setPassword((const char *)"###");
  
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("OTA Ready");
}
