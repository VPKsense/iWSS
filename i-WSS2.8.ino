#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#include <EEPROM.h>

//char auth[] = "oKPdLYfuKUOkgUS6bhhNL43oetoQNBTj";
char auth[]="8Dh6_WN6uGtFQCc-J4N2QgiVXuuSke7p";// test
char ssid[] = "2PoInT0";
char pass[] = "akhilesh";

//////V pins Define//////
#define Gatep V2
#define Sitoutp V3
#define MSRp V4
#define MSRledp V5
#define SSTp V6
#define SSTsetp V7
#define RARp V8
#define RARsetp V9
#define WBHp V10
#define WBHsetp V11
#define WBHselp V12
#define Buzztestp V13
#define Pingp V14
#define Timep V15
#define GNp V16
#define GNsetp V17
////////////////////////

///Support variables/////
int pstat=0; 
int MSRs=0,curpowerstat;
int SST=0,SSTset;
int Gate,Sitout;
int RAR,RARset;
int WBH,WBHset,WBHsel;
int GNset,GN;
int timeID;
////////////////////////

WidgetRTC rtc;
BlynkTimer timer; //Delay till final light after sunset

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

////////////////MAINS STATUS REPORTER (ALPHA)/////////////////

WidgetLED led(MSRledp);

void Powernotify()//MSR main function
{
  curpowerstat=EEPROM.read(1);
  if(curpowerstat==1)
  {
  Blynk.notify("Mains Power has been restored");
  curpowerstat=0;
  EEPROM.write(1,curpowerstat);
  EEPROM.commit();
  led.on();
  }
  else if(curpowerstat==0)
  {
  Blynk.notify("Mains Power has been lost");
  curpowerstat=1;
  EEPROM.write(1,curpowerstat);
  EEPROM.commit();
  led.off();
  }
}

BLYNK_WRITE(MSRp)//Switch to enable Mains Status Reporter 
{ MSRs=param.asInt();
if(MSRs)
{Powernotify();}
}

///////////////////////////////////////////////////////////////

////////////////////////SunSet Trigger/////////////////////////

BLYNK_WRITE(SSTp)//SST main
{
  SST=param.asInt();
  if((SST==5)&&(SSTset==1))
  {
  timeID= timer.setInterval(26*60*1000,SSTmain); //Switch ON light 26 minutes after sunset
  }
}

BLYNK_WRITE(SSTsetp)//SST switch
{ SSTset=param.asInt();
}

void SSTmain()//Switching function
{
  Blynk.virtualWrite(Sitoutp,1);
  digitalWrite(D5,LOW);
  Blynk.notify("Good Evening! Sit out light has been turned ON"); 
  timer.disable(timeID);
}

///////////////////////////////////////////////////////////////

////////////////////////Rain Alert Reporter/////////////////////////

BLYNK_WRITE(RARp)//RAR main
{
  RAR=param.asInt();
  if((RAR==5)&&(RARset==1))
  {
    Blynk.notify("Looks like it's gonna rain at your home");
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

BLYNK_WRITE(WBHp)//WBH main
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
}

////////////////////////////////////////////////////////////////

////////////////////////Good Night///////////////////////////////

BLYNK_WRITE(GNp)//GN main
{
  GN=param.asInt();
 if((GN==7)&&(GNset==1))
 {Blynk.virtualWrite(Sitoutp,0);
  Blynk.notify("Good night! Sit out light has been turned OFF");
  digitalWrite(D5,HIGH);
 }
}

BLYNK_WRITE(GNsetp)//GN switch
{
  GNset=param.asInt();
}

////////////////////////////////////////////////////////////////

//////////PING TEST///////////

BLYNK_WRITE(Pingp)
{int ping=param.asInt();
if(ping)
{ Blynk.virtualWrite(Timep, String(hour())+ " : " + minute() );
  Blynk.notify("Ping success!");
}  
}
  
//////////////////////////////

BLYNK_CONNECTED()
{ if(pstat==0)
  {
  rtc.begin();
  Blynk.notify("I'm Online :-)");
  delay(5000);
  Blynk.syncVirtual(Gatep,Sitoutp,MSRp,SSTsetp,RARsetp,WBHsetp,WBHselp,GNsetp);
  pstat++;
  }
else{
Blynk.notify("I've Reconnected to the server"); 
Blynk.syncVirtual(Gatep,Sitoutp,SSTsetp,RARsetp,WBHsetp,WBHselp,GNsetp);}
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v1.3 for i-WSS-");
  Serial.println("Booting up...");
  Blynk.begin(auth, ssid, pass);
  setSyncInterval(30 * 60);
  EEPROM.begin(3);  
  pinMode(D3,OUTPUT);//gate
  pinMode(D5,OUTPUT);//sitout
  delay(2000);
}

void loop()
{
  Blynk.run();
  timer.run();
}  
