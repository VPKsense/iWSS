#define BLYNK_PRINT Serial

#include <Dusk2Dawn.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>

char auth[] = "oKPdLYfuKUOkgUS6bhhNL43oetoQNBTj";
char ssid[] = "2PoInT1";
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
#define WBHp V10
#define WBHsetp V11
#define WBHselp V12
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
int CurTime,SSTset,SSTime,SSTcheck;
int Gate,Sitout;
int RAR,RARset;
int WBH,WBHset,WBHsel;
int GNset,GN;
int conbuz=0;
int hr,mt;
int conbuzstat;
int DisCount=0;
int SRTime,SRTcount=0;
String Condition;
int WeatherComp=0;
int WeatherCompP=0;
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
{Blynk.notify("Looks like Mains Power got interrupted");
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
if(WeatherCompP)
  SSTime-=20;
}

BLYNK_WRITE(SRTp)//SRT count store
{ SRTcount=param.asInt();
}

BLYNK_WRITE(WeaConp)// To compensate power loss in case of Compensated SST
{
  WeatherCompP=param.asInt();
}


BLYNK_WRITE(WeaCon)//SST Weather condition compensation
{
  Condition=param.asStr();
  if(Condition == "Haze" || Condition == "Cloudy" || Condition == "Fog" || Condition == "Rain")
  {
    SSTime-=20;
    char SSTime24[] = "00:00";
    Dusk2Dawn::min2str(SSTime24, SSTime);
    Blynk.virtualWrite(SSTimeCheck,String(SSTime24)+" C");
    WeatherComp=1;
    Blynk.virtualWrite(WeaConp,1);
  }
  else
  {
    WeatherComp=0;
    Blynk.virtualWrite(WeaConp,0);
  }
}

void SSTmain()//Switching function
{
  if(SSTset)
  {
  Blynk.virtualWrite(Sitoutp,1);
  digitalWrite(D5,LOW);
  if(!WeatherComp)
  Blynk.notify("Good Evening! Sit out light has been turned ON");
  else
  Blynk.notify("Good Evening! Sit out light has been turned ON early due to "+Condition);
  } 
}

void SSTCheck()//SST main & Time keeper
{
  hr=hour();
  mt=minute();
  CurTime= (hr*60)+ mt;
  char CurTime24[] = "00:00";
  Dusk2Dawn::min2str(CurTime24, CurTime);
  Blynk.virtualWrite(Timep, CurTime24 );
  
  if((CurTime>=SSTime)&&(SSTcheck==5))//Sunset checker
  {
   SSTmain(); 
   SSTcheck=7;
   Blynk.virtualWrite(SSTp,7);
  }

  if((CurTime>=SRTime)&&(SRTcount<5))//Sunrise checker
  {
   SRTcrow();
   SRTcount++;
   Blynk.virtualWrite(SRTp,SRTcount);
  }
  
  if((hr<1)&&(SSTcheck==7))//get sunset time for NEW day
  {
    SSTime= Home.sunset(year(), month(), day(), false)+30+10;//30 for time zone and 10 for approx last light
    SRTime= Home.sunrise(year(), month(), day(), false)+30+10;
    SRTcount=0;
    Blynk.virtualWrite(SRTp,SRTcount);//Reset Crow Count
    Blynk.virtualWrite(WeaConp,0);//Reset Weather Compensation pin
    SSTcheck=5;
    char SSTime24[] = "00:00";
    Dusk2Dawn::min2str(SSTime24, SSTime);
    Blynk.virtualWrite(SSTp,5);
    Blynk.virtualWrite(SSTimeCheck,SSTime24);//To get Sunset time on app
    DisCount=0; //Reset disconnection counter to 0 at the start of new day
    Blynk.virtualWrite(DisCounterp,DisCount);
  }
}

void SRTcrow()
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
}



///////////////////////////////////////////////////////////////

////////////////////////Rain Alert Reporter/////////////////////////

BLYNK_WRITE(RARp)//RAR main
{
  RAR=param.asInt();
  if((RAR==5)&&(RARset==1))
  {
    hr=hour();
    mt=minute();
    CurTime= (hr*60)+ mt;
    char CurTime24[] = "00:00";
    Dusk2Dawn::min2str(CurTime24, CurTime);
    Blynk.notify("Looks like it's gonna rain at your home (Time: "+ String(CurTime24) + ")");
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


///////////////////////Good Night///////////////////////////////

BLYNK_WRITE(GNp)//GN main
{
  GN=param.asInt();
 if((GN==7)&&(GNset==1))
 {Blynk.virtualWrite(Sitoutp,0);
  Blynk.notify("Good night! Sit out light has been turned OFF");
  digitalWrite(D5,HIGH);
  Blynk.virtualWrite(GNp,5);
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
if(ping==1)
{ 
 Blynk.notify("Ping success!");
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
{ if(pstat==0)
  {
  rtc.begin();
  Blynk.notify("I'm ready :-)");
  delay(5000);
  Blynk.syncVirtual(Gatep,Sitoutp,MSRp,WeaConp,SSTp,SSTsetp,SSTimeCheck,RARsetp,WBHsetp,WBHselp,GNsetp,GNp,Buzsetp,DisCounterp,SRTp);
  pstat++;
  digitalWrite(D4,HIGH);
  }
else{
 if(conbuzstat==1){
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
 Blynk.notify("I've Reconnected");} 
Blynk.syncVirtual(Gatep,Sitoutp,SSTsetp,SSTp,RARsetp,WBHsetp,WBHselp,GNsetp,GNp,Buzsetp);
digitalWrite(D4,HIGH);
conbuz=0;
DisCount++;
Blynk.virtualWrite(DisCounterp,DisCount); //Count no. of disconnections in  a day
    }
}

void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println("               -丂𝐞𝐧𝐬𝐞 𝐎𝐒 v1.7.3 for i-WSS-");
  Serial.println("Booting up...");
  pinMode(D4,OUTPUT);//Noconnection LED
  digitalWrite(D4,LOW);
  Blynk.begin(auth, ssid, pass,IPAddress(188,166,206,43),8080);
  setSyncInterval(120 * 60);// 2 hr
  timer.setInterval(30*1000, SSTCheck);//30 seconds
  pinMode(D3,OUTPUT);//gate
  pinMode(D5,OUTPUT);//sitout
  delay(2000);
}

void loop()
{
  timer.run();
  Blynk.run();
  if (!Blynk.connected()) 
  {  
    digitalWrite(D4,LOW);
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
