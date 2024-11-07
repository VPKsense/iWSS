#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPLAkPzhMB_"
#define BLYNK_TEMPLATE_NAME "iWSS D"
#define BLYNK_AUTH_TOKEN "###"

#include <BlynkSimpleEsp8266.h>
#include <Dusk2Dawn.h>
#include <WidgetRTC.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "###";
char pass[] = "###";
const char *OTApass = "###";

//////V pins Define//////
#define Gatep V2
#define Sitoutp V3
#define MSRp V4
#define Buzsetp V5
#define SSTp V6 
#define SSTsetp V7
#define RARp V8
#define RARsetp V9
#define Buzztestp V13
#define Pingp V14
#define Timep V15
#define GNp V16
#define GNsetp V17
#define SSTimeCheck V18 
#define DisCounterp V19
#define SRTp V21
#define WeaConp V22
#define Floodp V23
#define SitoutUp V24
#define AllOn V25
////////////////////////

///////Crow Notes///////
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
//A AS AS AS-GS
////////////////////////

//////Digital Pins//////
#define OnBoardLed D4
#define RLed D0
#define GLed D1
#define BLed D2
#define SitoutDP D3
#define GateDP D5
#define FloodDP D6
#define BuzzDP D8
////////////////////////

///Support variables/////
int pstat=0; 
int MSRs=0,curpowerstat;
int CurTime,SSTset,SSTime,SSTcheck;
int Gate,Sitout;
int RAR,RARset;
int GNset,GN;
int GNTime=1305;//9:45 PM
int conbuz=0;
int hr,mt;
int conbuzstat;
int DisCount=0;
int SRTime,SRTcount=0,CrowAlt=true;
int WeatherComp=0;
int pflag=0,sflag=0;
int Shr,Smin;
int FloodS=0;
int CloudCover=0,Raining=0, WeaCompCheck=0;
String WeatherCondition;
int ReqSuccess = 0;
////////////////////////

WidgetRTC rtc;
BlynkTimer timer; //For SST check

//////////////////////Switch Controllers///////////////////////

BLYNK_WRITE(Gatep)
{ Gate=param.asInt();
if(Gate)
{digitalWrite(GateDP,LOW);
}
else if(!Gate)
{digitalWrite(GateDP,HIGH);}
}

BLYNK_WRITE(Sitoutp)
{ Sitout=param.asInt();
if(Sitout)
{digitalWrite(SitoutDP,LOW);
}
else if(!Sitout)
{digitalWrite(SitoutDP,HIGH);
}
}

//////////////////////////////////////////////////////////////

//////////////MAINS STATUS REPORTER (iWSS only)///////////////

BLYNK_WRITE(MSRp)//Switch to enable Mains Status Reporter 
{ MSRs=param.asInt();
if(MSRs)
{Blynk.logEvent("mains","Looks like iWSS Power got interrupted");
}
}

///////////////////////////////////////////////////////////////

////////////////////////SunSet Trigger/////////////////////////

Dusk2Dawn Home(10.324344,76.200689, 5);//Coordinates for sunset

BLYNK_WRITE(SSTsetp)//SST switch
{ SSTset=param.asInt();
}

BLYNK_WRITE(SSTp)//SST store value
{
  SSTcheck=param.asInt();
}

BLYNK_WRITE(SSTimeCheck)//SST time store value
{
  SSTime= Home.sunset(year(), month(), day(), false)+30+10;//30 for time zone and 10 for approx last light
  if(WeatherComp==1)
    SSTime-=10;
  else if(WeatherComp==2)
    SSTime-=20;
}

BLYNK_WRITE(SRTp)//SRT count store
{ SRTcount=param.asInt();
}

BLYNK_WRITE(WeaConp)// To compensate power loss in case of Compensated SST
{
  WeatherComp=param.asInt();
}


void WeatherCompCheck()//SST Weather condition compensation
{
  WeatherCheck();
  if(CloudCover >= 50 && CloudCover < 80)
  {
    SSTime-=10;
    char SSTime24[] = "00:00";
    Dusk2Dawn::min2str(SSTime24, SSTime);
    Blynk.virtualWrite(SSTimeCheck,String(SSTime24)+" C1");
    WeatherComp=1;
    Blynk.virtualWrite(WeaConp,WeatherComp);
  }
  else if(CloudCover >= 80)
  {
    SSTime-=20;
    char SSTime24[] = "00:00";
    Dusk2Dawn::min2str(SSTime24, SSTime);
    Blynk.virtualWrite(SSTimeCheck,String(SSTime24)+" C2");
    WeatherComp=2;
    Blynk.virtualWrite(WeaConp,WeatherComp);
  }
  else
  {
    WeatherComp=0;
    Blynk.virtualWrite(WeaConp,WeatherComp);
  }
  
  EEPROM.write(5,SSTime/60);
  EEPROM.write(6,SSTime%60);
  EEPROM.commit();  
}

void SSTmain()//Switching function
{
  if(SSTset)
  {
  Blynk.virtualWrite(Sitoutp,1);
  digitalWrite(SitoutDP,LOW);
  if(!WeatherComp)
  Blynk.logEvent("lights","Good Evening! Sit out light has been turned ON");
  else
  Blynk.logEvent("lights",String("Good Evening! Sit out light has been turned ON early due to ") + WeatherCondition);
  } 
}

void MainCheck()//SST main & Time keeper
{
  if(!Blynk.connected() && !pflag)//Restart at No Internet
  {
    
    if(!sflag)
    {
      Serial.println("No internet! Seeding RTC");
      setTime(int(EEPROM.read(0)),int(EEPROM.read(1)),0,int(EEPROM.read(2)),int(EEPROM.read(3)),int(EEPROM.read(4)));
      Shr=EEPROM.read(5);
      Smin=EEPROM.read(6);
      SSTime=(Shr*60)+Smin;
      Serial.println(SSTime);

      sflag=1;
    } 
    hr=hour();
    mt=minute();
    CurTime= (hr*60)+ mt;
    EEPROM.write(0,hour());
    EEPROM.write(1,minute());
    EEPROM.write(2,day());
    EEPROM.write(3,month());
    EEPROM.write(4,year());
    EEPROM.commit();

    if((CurTime>=SSTime)&&(SSTcheck==1))
    {
      digitalWrite(SitoutDP,LOW);
      SSTcheck=0;
    }
 
    if((CurTime>=GNTime)&&(GN==0))
    {
      digitalWrite(SitoutDP,HIGH);
      GN=1;
    }
    
    if((hr<1)&&(SSTcheck==0))//get sunset time for NEW day and Reset values
    {
      SSTime= Home.sunset(year(), month(), day(), false)+30+10;//30 for time zone and 10 for approx last light
      GN=0;// Reset GNpin
      SSTcheck=1;
      WeaCompCheck=0;
      EEPROM.write(5,SSTime/60);
      EEPROM.write(6,SSTime%60);
      EEPROM.commit();
    }
  }
  else
  {
    hr=hour();
    mt=minute();
    CurTime= (hr*60)+ mt;
    char CurTime24[] = "00:00";
    Dusk2Dawn::min2str(CurTime24, CurTime);
    Blynk.virtualWrite(Timep, CurTime24 );

    if((CurTime>= (SSTime-30)) && !WeaCompCheck)//Weather compensation before 30min of SST
    {
      WeatherCompCheck();
      if(ReqSuccess)
        WeaCompCheck=1;
    }
    
    if((CurTime>=SSTime)&&(SSTcheck==1))//Sunset checker
    {
     SSTmain(); 
     SSTcheck=0;
     Blynk.virtualWrite(SSTp,SSTcheck);
    }
  
    if((CurTime>=SRTime)&&(SRTcount<5))//Sunrise checker
    {
     SRTcrow();
     Blynk.virtualWrite(SRTp,SRTcount);
    }
  
    if((CurTime>=GNTime)&&(GN==0))//GN checker (GN Time defined in variables)
    {
     GNlight();
    }
    
    if((hr<1)&&(SSTcheck==0))//get sunset time for NEW day and Reset values
    {
      SSTime= Home.sunset(year(), month(), day(), false)+30+10;//30 for time zone and 10 for approx last light
      SRTime= Home.sunrise(year(), month(), day(), false)+30-10;// -10 for first light
      SRTcount=0;
      Blynk.virtualWrite(SRTp,SRTcount);//Reset Crow Count
      WeatherComp=0;
      WeaCompCheck=0;
      Blynk.virtualWrite(WeaConp,WeatherComp);//Reset Weather Compensation pin
      GN=0;// Reset GNpin
      Blynk.virtualWrite(GNp,GN);
      SSTcheck=1;
      char SSTime24[] = "00:00";
      Dusk2Dawn::min2str(SSTime24, SSTime);
      Blynk.virtualWrite(SSTp,SSTcheck);
      Blynk.virtualWrite(SSTimeCheck,SSTime24);//To get Sunset time on app
      DisCount=0; //Reset disconnection counter to 0 at the start of new day
      Blynk.virtualWrite(DisCounterp,DisCount);
      EEPROM.write(5,SSTime/60);
      EEPROM.write(6,SSTime%60);
      EEPROM.commit();
    }
    EEPROM.write(0,hour());
    EEPROM.write(1,minute());
    EEPROM.write(2,day());
    EEPROM.write(3,month());
    EEPROM.write(4,year());
    EEPROM.commit();

    if(Blynk.connected())
      UpperStatusChecker(0);
  }
}

void SRTcrow()
{
  if(CrowAlt)
  {
    tone(BuzzDP,NOTE_A7);
    delay(300);
    noTone(BuzzDP);
    delay(100);
    tone(BuzzDP,NOTE_AS7);
    delay(250);
    noTone(BuzzDP);
    delay(30);
    tone(BuzzDP,NOTE_AS7);
    delay(250);
    noTone(BuzzDP);
    delay(150);
    tone(BuzzDP,NOTE_AS7);
    delay(500);
    for(int i=NOTE_AS7;i>=NOTE_GS7;i-=2)
    {
      tone(BuzzDP,i);
      delay(8);
      }
    noTone(BuzzDP);
    SRTcount++;
  }
  CrowAlt=!CrowAlt;// Increase time between crows to 1 minute
}

///////////////////////////////////////////////////////////////

////////////////////////Rain Alert Reporter/////////////////////////

BLYNK_WRITE(RARp)//RAR main
{
  RAR=param.asInt();
  if((RAR==1)&&(RARset==1))
  {
    hr=hour();
    mt=minute();
    CurTime= (hr*60)+ mt;
    char CurTime24[] = "00:00";
    Dusk2Dawn::min2str(CurTime24, CurTime);
    Blynk.logEvent("rain",String("Looks like it's gonna rain at your home (Time: ")+ String(CurTime24) + ")");
    for(int k=0;k<5;k++)
    {
        tone(BuzzDP,4000);
        delay(3000);
        noTone(BuzzDP);
        delay(500);
        tone(BuzzDP,4000);
        delay(150);
        noTone(BuzzDP);
        delay(80);
        tone(BuzzDP,4000);
        delay(150);
        noTone(BuzzDP);
        delay(1000);
    }
  }
}

BLYNK_WRITE(RARsetp)//RAR switch
{
  RARset=param.asInt();
}

BLYNK_WRITE(Buzztestp)
{
  int buzz=param.asInt();
  if(buzz)
  {
    tone(BuzzDP,4000);
  }
  else {
    noTone(BuzzDP);
  }
}

////////////////////////////////////////////////////////////////////

///////////////////////Good Night///////////////////////////////

BLYNK_WRITE(GNp)//GN status store
{
  GN=param.asInt();
}

BLYNK_WRITE(GNsetp)//GN switch
{
  GNset=param.asInt();
}

void GNlight()//GN main
{
   if(GNset)
   {
    Blynk.virtualWrite(Sitoutp,0);
    Blynk.logEvent("lights","Good night! Sit out light has been turned OFF");
    digitalWrite(SitoutDP,HIGH);
    GN=1;
    Blynk.virtualWrite(GNp,GN);
   }
}

////////////////////////////////////////////////////////////////

//////////PING TEST///////////

BLYNK_WRITE(Pingp)
{
  int ping=param.asInt();
  if(ping)
  { 
    Blynk.logEvent("internet","Ping success!");
  }  
}
  
//////////////////////////////

//////////Disconection Buzzer+Counter///////////

BLYNK_WRITE(Buzsetp){
  conbuzstat=param.asInt();
}

BLYNK_WRITE(DisCounterp){
  DisCount=param.asInt();
}
  
///////////////////////////////////////////////

////////////////////ALL ON////////////////////

BLYNK_WRITE(AllOn)
{
  int allon=param.asInt();
  if(allon)
  {
    Blynk.virtualWrite(Sitoutp,1);
    digitalWrite(SitoutDP,LOW);
    delay(100);
    Blynk.virtualWrite(Gatep,1);
    digitalWrite(GateDP,LOW);
    delay(100);    
    Blynk.virtualWrite(Floodp,1);
    delay(150);
    Blynk.virtualWrite(SitoutUp,1);
  }
  else
  {
    Blynk.virtualWrite(Sitoutp,0);
    digitalWrite(SitoutDP,HIGH);
    delay(100);    
    Blynk.virtualWrite(Gatep,0);
    digitalWrite(GateDP,HIGH);
    delay(100);
    Blynk.virtualWrite(Floodp,0);
    delay(150);    
    Blynk.virtualWrite(SitoutUp,0);
  }
}
  
//////////////////////////////////////////////

////////////Flood Light///////////////
void SwitchCheck()
{
  if (Blynk.connected())
  {
    if(!FloodS && !digitalRead(FloodDP))//for flood light
    {
      delay(300);
      UpperStatusChecker(1);
      if(!digitalRead(FloodDP))
      {
        Blynk.virtualWrite(Floodp,1);
        FloodS=1;
      }
    }
    else if(FloodS && digitalRead(FloodDP))
    {
      delay(300);
      UpperStatusChecker(1);
      if(digitalRead(FloodDP))
      {
        Blynk.virtualWrite(Floodp,0);
        FloodS=0;
      }
    }
  }
}

BLYNK_WRITE(Floodp)
{
  FloodS = param.asInt();
}

void UpperStatusChecker(int FromSwitch)//Checks the online status of iWSS(U)
{
  HTTPClient http;
  WiFiClient client;
  String url = "http://68.183.87.221/external/api/isHardwareConnected?token=XesCJ7YxSjCeLQg6y7Df8qss7ZrjYvM0"; 
  
  http.begin(client,url);
  int httpCode = http.GET();

  if(httpCode == HTTP_CODE_OK)
  {
    String RData = http.getString();
    if(RData == "true")
      {
        digitalWrite(RLed,LOW);
      }
    else
      {
        digitalWrite(RLed,HIGH);
        digitalWrite(GLed,LOW);
        if(FromSwitch)
        {
          tone(BuzzDP,4000);
          delay(150);
          noTone(BuzzDP);
          delay(80);
          tone(BuzzDP,4000);
          delay(150);
          noTone(BuzzDP);
          delay(500);
        }
      }
  }
  else
    Serial.println("HTTP response error: " + String(httpCode));
  http.end();
}

//////////////////////////////////////

/////////////////Weather Checker and Rain monitor/////////////////
void WeatherCheck()
{
  if(Blynk.connected())
  {
    HTTPClient http;
    WiFiClient client;
    String url = "http://api.weatherapi.com/v1/current.json?key=f3addcfaa4d34660826125743240308&q=10.3243,76.2007&aqi=no"; 

    http.begin(client,url);
    int httpCode = http.GET(); // Make the request

    if (httpCode == HTTP_CODE_OK)
    {
      ReqSuccess = 1;
      String payload = http.getString();
      
      JsonDocument doc;// Parse JSON object
      DeserializationError error = deserializeJson(doc, payload);
      if (error) {
        Serial.print("deserializeJson() failed: "+ String(error.f_str()));
        return;
      }
      CloudCover = int(doc["current"]["cloud"]);//Extract
      WeatherCondition = String(doc["current"]["condition"]["text"]);
      if(CloudCover >= 75 && ((WeatherCondition.indexOf("drizzle")!=-1 && CloudCover >=80) 
      || WeatherCondition.indexOf("rain")!=-1 || WeatherCondition.indexOf("Rain")!=-1)) //Rain monitor
      {
        if(!Raining)
        {
          RainAlertWR();
          Raining = 1;
          digitalWrite(GLed,HIGH);
        }
      }
      else if (CloudCover <= 65 || (WeatherCondition.indexOf("drizzle")!=-1 && CloudCover <=70))
      {
        Raining = 0;
        digitalWrite(GLed,LOW);
      }
    } 
    else 
      {
        ReqSuccess = 0;//Error
        Serial.println("Error on HTTP request: " + String(httpCode));
      }
    http.end(); // Close connection
  }
}

void RainAlertWR() 
{
  HTTPClient http;
  WiFiClient client;
  String url = "http://68.183.87.221/external/api/update?token=2LLVZj-x3_DBoQEd7Oc2B40XJJMqggGk&V8=1"; 
  
  http.begin(client,url);
  int httpCode = http.GET();

  if(httpCode != HTTP_CODE_OK)
    Serial.println("HTTP response error: " + String(httpCode));

  http.end();
}

//////////////////////////////////////////////////////////////////

BLYNK_CONNECTED()
{ 
  if(!pstat)
  {
  rtc.begin();
  delay(1000);
  Blynk.syncVirtual(Gatep,Sitoutp,MSRp,WeaConp,SSTp,SSTsetp,SSTimeCheck,RARsetp,GNsetp,GNp,Buzsetp,DisCounterp,SRTp,Floodp);
  pstat=1;
  pflag=1;//for Restart at No Internet
  digitalWrite(OnBoardLed,HIGH);
  digitalWrite(BLed,LOW);
  tone(BuzzDP,4000);//Connection buzzer
  delay(150);
  noTone(BuzzDP);
  delay(80);
  tone(BuzzDP,4000);
  delay(150);
  noTone(BuzzDP);
  delay(80);
  tone(BuzzDP,4000);
  delay(150);
  noTone(BuzzDP);
  }
  else
  {
    if(conbuzstat==1)
    {
      tone(BuzzDP,4000);//Reconnection buzzer
      delay(150);
      noTone(BuzzDP);
      delay(80);
      tone(BuzzDP,4000);
      delay(150);
      noTone(BuzzDP);
      delay(200);
      tone(BuzzDP,4000);
      delay(1500);
      noTone(BuzzDP);
      Blynk.logEvent("internet","I've Reconnected");
    } 
    Blynk.syncVirtual(Gatep,Sitoutp,SSTsetp,SSTp,RARsetp,GNsetp,GNp,Buzsetp,Floodp);
    digitalWrite(OnBoardLed,HIGH);
    digitalWrite(BLed,LOW);
    conbuz=0;
    DisCount++;
    Blynk.virtualWrite(DisCounterp,DisCount); //Count no. of disconnections in  a day
    if(Raining)
      digitalWrite(GLed,HIGH);
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v2.0.7 for i-WSS(D)-");
  Serial.println("Booting up...");
  pinMode(OnBoardLed,OUTPUT);//No connection LED
  pinMode(BLed,OUTPUT);
  digitalWrite(BLed,HIGH);
  pinMode(RLed,OUTPUT);//Upper Status LED
  pinMode(GLed,OUTPUT);//Rain LED
  pinMode(FloodDP, INPUT_PULLUP);
  EEPROM.begin(20);
  OTA();
  Blynk.connectWiFi(ssid, pass);
  Blynk.config(auth);
  Blynk.connect(5);
  setSyncInterval(30 * 60);// 30minutes
  timer.setInterval(30*1000, MainCheck);//For main functionality - 30s
  timer.setInterval(1*1000, SwitchCheck);//For flood light functionality - 1s  
  timer.setInterval(60*1000*16, WeatherCheck);//For weather check - rain & cloud cover - 16min
  pinMode(GateDP,OUTPUT);
  pinMode(SitoutDP,OUTPUT);
  delay(3000);
  if(!Blynk.connected())
  {
    digitalWrite(SitoutDP,HIGH);
    digitalWrite(GateDP,HIGH);
  }
}


void loop()
{
  Blynk.run();
  timer.run();
  ArduinoOTA.handle();
  if (!Blynk.connected()) 
  {  
    digitalWrite(OnBoardLed,LOW);
    digitalWrite(BLed,HIGH);
    digitalWrite(RLed,LOW);
    digitalWrite(GLed,LOW);
    if((conbuz==0)&&(conbuzstat==1))
    {
        for(int i=0;i<5;i++)
        {tone(BuzzDP,4000);
        delay(150);
        noTone(BuzzDP);
        delay(80);
        tone(BuzzDP,4000);
        delay(150);
        noTone(BuzzDP);
        delay(500);}
    conbuz=1;
    }
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

  ArduinoOTA.setHostname("i-WSS-D");
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
