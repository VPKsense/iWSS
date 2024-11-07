#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPLiwzn1ooT"
#define BLYNK_TEMPLATE_NAME "iWSS U"
#define BLYNK_AUTH_TOKEN "###"

#include <BlynkSimpleEsp8266.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "###";
char pass[] = "###";
const char *OTApass = "###";

//////V pins Define//////
#define Sitoutp V1
#define Floodp V2
#define RSSIp V3
////////////////////////

//////Digital Pins//////
#define OnBoardLed1 D0
#define OnBoardLed2 D4
#define FloodDP D3
#define SitoutDP D5
////////////////////////

///Support variables/////
int pstat=0; 
int dat;
long rssi;
////////////////////////


BLYNK_WRITE(Floodp)
{
  dat=param.asInt();
  if(dat)
  digitalWrite(FloodDP,LOW);
  else
  digitalWrite(FloodDP,HIGH);
}

BLYNK_WRITE(Sitoutp)
{
  dat=param.asInt();
  if(dat)
  digitalWrite(SitoutDP,LOW);
  else
  digitalWrite(SitoutDP,HIGH);
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
  digitalWrite(OnBoardLed2,HIGH);
  digitalWrite(OnBoardLed1,HIGH);
  }
  else
  {
    Blynk.syncVirtual(Sitoutp,Floodp);
    digitalWrite(OnBoardLed2,HIGH);
    digitalWrite(OnBoardLed1,HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v1.0.3 for i-WSS(U)-");
  Serial.println("Booting up...");
  pinMode(OnBoardLed2,OUTPUT);//Noconnection LED
  pinMode(OnBoardLed1,OUTPUT);
  OTA();
  Blynk.connectWiFi(ssid, pass);
  Blynk.config(auth);
  Blynk.connect(5);
  pinMode(FloodDP,OUTPUT);
  digitalWrite(FloodDP,HIGH);
  pinMode(SitoutDP,OUTPUT);
  digitalWrite(SitoutDP,HIGH);
  delay(3000);
}

void loop()
{
  Blynk.run();
  ArduinoOTA.handle();
  if (!Blynk.connected()) 
  {  
    digitalWrite(OnBoardLed2,LOW);
    digitalWrite(OnBoardLed1,LOW);
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
  ArduinoOTA.setPassword(OTApass);
  
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
