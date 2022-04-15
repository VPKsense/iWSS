
#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

char auth[] = "oKPdLYfuKUOkgUS6bhhNL43oetoQNBTj";
char ssid[] = "2PoInT0";
char pass[] = "akhilesh";

BLYNK_CONNECTED()
{delay(1500);
Blynk.syncAll();
}

void setup()
{
  Serial.begin(9600);
  Blynk.begin(auth, ssid, pass);
  Blynk.notify("I'm Online!");
}

void loop()
{
  Blynk.run();
}
