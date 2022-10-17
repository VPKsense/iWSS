#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPLAkPzhMB_"
#define BLYNK_DEVICE_NAME "iWSS"
#define BLYNK_AUTH_TOKEN "2LLVZj-x3_DBoQEd7Oc2B40XJJMqggGk"

#include <Dusk2Dawn.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "2PoInT0";
char pass[] = "akhilesh";

//////V pins Define//////
#define Gatep V2
#define Sitoutp V3
#define MSRp V4
#define Buzsetp V5
#define SSTp V6 
#define SSTsetp V7
#define RARp V8
#define RARsetp V9
//#define WBHp V10
//#define WBHsetp V11
//#define WBHselp V12
#define Buzztestp V13
#define Pingp V14
#define Timep V15
#define GNp V16
#define GNsetp V17
#define SSTimeCheck V18 
#define DisCounterp V19
#define WeaCon V20
#define SRTp V21
#define WeaConp V22
////////////////////////

///////Crow Notes///////
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
//A AS AS AS-GS
////////////////////////

///Support variables/////
int pstat=0; 
int MSRs=0,curpowerstat;
int CurTime,SSTset,SSTime,SSTcheck=2;
int Gate,Sitout;
int RAR,RARset;
//int WBH,WBHset,WBHsel;
int GNset,GN;
int GNTime=1305;//9:45 PM
int conbuz=0;
int hr,mt;
int conbuzstat;
int DisCount=0;
int SRTime,SRTcount=0,CrowAlt=true;
String Condition;
int WeatherComp=0;
int pflag=0,sflag=0;
int Shr,Smin;
////////////////////////

WidgetRTC rtc;
BlynkTimer timer; //For SST check

//////////////////////Switch Controllers///////////////////////

BLYNK_WRITE(Gatep)
{ Gate=param.asInt();
if(Gate)
{digitalWrite(D3,LOW);
}
else if(!Gate)
{digitalWrite(D3,HIGH);}
}

BLYNK_WRITE(Sitoutp)
{ Sitout=param.asInt();
if(Sitout)
{digitalWrite(D5,LOW);
}
else if(!Sitout)
{digitalWrite(D5,HIGH);
}
}

//////////////////////////////////////////////////////////////

////////////////MAINS STATUS REPORTER (BETA)//////////////////

BLYNK_WRITE(MSRp)//Switch to enable Mains Status Reporter 
{ MSRs=param.asInt();
if(MSRs)
{Blynk.logEvent("mains","Looks like Mains Power got interrupted");
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


BLYNK_WRITE(WeaCon)//SST Weather condition compensation
{
  Condition=param.asStr();
  if(Condition == "Haze" || Condition == "Mostly Cloudy" || Condition == "Partly Cloudy")
  {
    SSTime-=10;
    char SSTime24[] = "00:00";
    Dusk2Dawn::min2str(SSTime24, SSTime);
    Blynk.virtualWrite(SSTimeCheck,String(SSTime24)+" C1");
    WeatherComp=1;
    Blynk.virtualWrite(WeaConp,WeatherComp);
  }
  else if(Condition == "Cloudy" || Condition == "Fog" || Condition == "Rain" || Condition == "Light Drizzle" || 
          Condition == "Drizzle" || Condition == "Thunderstorm" || Condition == "Light Rain" || Condition == "Heavy Rain")
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
  
  EEPROM.write(4,SSTime/60);
  EEPROM.write(5,SSTime%60);
  EEPROM.commit();  
}

void SSTmain()//Switching function
{
  if(SSTset)
  {
  Blynk.virtualWrite(Sitoutp,1);
  digitalWrite(D5,LOW);
  if(!WeatherComp)
  Blynk.logEvent("lights","Good Evening! Sit out light has been turned ON");
  else
  Blynk.logEvent("lights",String("Good Evening! Sit out light has been turned ON early due to ")+Condition);
  } 
}

void MainCheck()//SST main & Time keeper
{
  if(!Blynk.connected() && !pflag)//Restart at No Internet
  {
    
    if(!sflag)
    {
      Serial.println("No internet! Seeding RTC");
      setTime(int(EEPROM.read(0)),int(EEPROM.read(1)),0,int(EEPROM.read(2)),int(EEPROM.read(3)),2022);
      Shr=EEPROM.read(4);
      Smin=EEPROM.read(5);
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
    EEPROM.commit();

    if(CurTime>=SSTime)
    digitalWrite(D5,LOW);
 
    if(CurTime>=GNTime)
    digitalWrite(D5,HIGH);
  }
  else
  {
    hr=hour();
    mt=minute();
    CurTime= (hr*60)+ mt;
    char CurTime24[] = "00:00";
    Dusk2Dawn::min2str(CurTime24, CurTime);
    Blynk.virtualWrite(Timep, CurTime24 );
    
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
      EEPROM.write(4,SSTime/60);
      EEPROM.write(5,SSTime%60);
      EEPROM.commit();
    }
    EEPROM.write(0,hour());
    EEPROM.write(1,minute());
    EEPROM.write(2,day());
    EEPROM.write(3,month());
    EEPROM.commit();
  }
}

void SRTcrow()
{
  if(CrowAlt)
  {
    tone(D8,NOTE_A7);
    delay(300);
    noTone(D8);
    delay(100);
    tone(D8,NOTE_AS7);
    delay(250);
    noTone(D8);
    delay(30);
    tone(D8,NOTE_AS7);
    delay(250);
    noTone(D8);
    delay(150);
    tone(D8,NOTE_AS7);
    delay(500);
    for(int i=NOTE_AS7;i>=NOTE_GS7;i-=2)
    {
      tone(D8,i);
      delay(8);
      }
    noTone(D8);
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
        tone(D8,4000);
        delay(3000);
        noTone(D8);
        delay(500);
        tone(D8,4000);
        delay(150);
        noTone(D8);
        delay(80);
        tone(D8,4000);
        delay(150);
        noTone(D8);
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
    tone(D8,4000);
  }
  else {
    noTone(D8);
  }
}

////////////////////////////////////////////////////////////////////

//////////////////////Welcome Back Home/////////////////////////

/*BLYNK_WRITE(WBHp)//WBH main
{
  WBH=param.asInt();
  if((WBH==1)&&(WBHset==1))
  {
    Blynk.notify("Welcome back home :-)");
    if(hour()>=18)
    {switch(WBHsel)
     {
      case 1: Blynk.virtualWrite(Gatep,1);
              digitalWrite(D3,LOW);
              break;
      case 2: Blynk.virtualWrite(Gatep,1);
              Blynk.virtualWrite(Sitoutp,1);
              digitalWrite(D3,LOW);
              delay(1000);
              digitalWrite(D5,LOW);
              break;        
     }
    }
  }
}

BLYNK_WRITE(WBHsetp)//WBH switch
{
  WBHset=param.asInt();
}

BLYNK_WRITE(WBHselp)//WBH selection
{
  WBHsel=param.asInt();
}*/

////////////////////////////////////////////////////////////////


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
    digitalWrite(D5,HIGH);
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

BLYNK_CONNECTED()
{ if(!pstat)
  {
  rtc.begin();
  delay(1000);
  Blynk.syncVirtual(Gatep,Sitoutp,MSRp,WeaConp,SSTp,SSTsetp,SSTimeCheck,RARsetp,/*WBHsetp,WBHselp,*/GNsetp,GNp,Buzsetp,DisCounterp,SRTp);
  pstat=1;
  pflag=1;//for Restart at No Internet
  digitalWrite(D4,HIGH);
  digitalWrite(D0,HIGH);
  tone(D8,4000);//Connection buzzer
  delay(150);
  noTone(D8);
  delay(80);
  tone(D8,4000);
  delay(150);
  noTone(D8);
  delay(80);
  tone(D8,4000);
  delay(150);
  noTone(D8);
  }
else{
 if(conbuzstat==1)
 {
 tone(D8,4000);//Reconnection buzzer
 delay(150);
 noTone(D8);
 delay(80);
 tone(D8,4000);
 delay(150);
 noTone(D8);
 delay(200);
 tone(D8,4000);
 delay(1500);
 noTone(D8);
 Blynk.logEvent("internet","I've Reconnected");
 } 
Blynk.syncVirtual(Gatep,Sitoutp,SSTsetp,SSTp,RARsetp,/*WBHsetp,WBHselp,*/GNsetp,GNp,Buzsetp);
digitalWrite(D4,HIGH);
digitalWrite(D0,HIGH);
conbuz=0;
DisCount++;
Blynk.virtualWrite(DisCounterp,DisCount); //Count no. of disconnections in  a day
    }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v1.8.4 for i-WSS-");
  Serial.println("Booting up...");
  pinMode(D4,OUTPUT);//Noconnection LED
  pinMode(D0,OUTPUT);
  EEPROM.begin(512);
  OTA();
  Blynk.connectWiFi(ssid, pass);
  Blynk.config(auth,IPAddress(68,183,87,221),8080);
  Blynk.connect(5);
  setSyncInterval(30 * 60);// 30minutes
  timer.setInterval(30*1000, MainCheck);//30 seconds
  pinMode(D3,OUTPUT);//gate
  pinMode(D5,OUTPUT);//sitout
  delay(3000);
  if(!Blynk.connected())
  {
    digitalWrite(D5,HIGH);
    digitalWrite(D3,HIGH);
  }
}

void loop()
{
  timer.run();
  Blynk.run();
  ArduinoOTA.handle();
  if (!Blynk.connected()) 
  {  
    digitalWrite(D4,LOW);
    digitalWrite(D0,LOW);
    if((conbuz==0)&&(conbuzstat==1))
    {
        for(int i=0;i<5;i++)
        {tone(D8,4000);
        delay(150);
        noTone(D8);
        delay(80);
        tone(D8,4000);
        delay(150);
        noTone(D8);
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

  ArduinoOTA.setHostname("i-WSS");
  ArduinoOTA.setPassword((const char *)"sensepro");
  
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
