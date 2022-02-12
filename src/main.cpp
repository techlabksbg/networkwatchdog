#include <Arduino.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <time.h>


// Wait for WiFi (max 5 min, then reboot)
// as long as wifi:
//     every 1 min test internet connection.
//     when failed for 5 min, power down modem, wait for 2 min, restart programm logic.
//     

#define RELAY 12
#define LED 13


bool connectToWifi(unsigned long timeoutSecs=30) {
  WiFi.disconnect(true);
  delay(10);
  WiFi.begin("tech-lab", "tech-lab");
  long unsigned int now = millis();
  while (millis()-now<1000uL*timeoutSecs && !(WiFi.status() == WL_CONNECTED)) {
    delay(200);
    ESP.wdtFeed();
    digitalWrite(LED, !digitalRead(LED));
  }
  digitalWrite(LED, HIGH);
  Serial.print("WiFi is ");
  Serial.println(WiFi.isConnected());
  if (WiFi.isConnected()) {
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }
  return WiFi.status() == WL_CONNECTED;
}

WiFiClient client;

bool can_haz_interweb() {
  const char* hosts[] = {"mx.bloechligair.ch", "fginfo.ksbg.ch", "linuxbox.ch"};
  for (int i=0; i<3; i++) {
    ESP.wdtFeed();
    client.connect(hosts[i], 80);
    ESP.wdtFeed();
    if (client.connected()) {
      int mins = millis()/60000;
      int hours = mins/60;
      int days = hours/24;
      mins %= 60;
      hours %= 24;
      client.printf("GET /techlab-watchdog-%dd-%02d:%02d HTTP/1.0\r\nHost:",days, hours, mins);
      client.print(hosts[i]);
      client.print("\r\n\r\n");
      Serial.print("Connection to ");
      Serial.print(hosts[i]);
      Serial.println(" established.");
      client.stop();
      return true;
    }
    Serial.print("Could not connect to ");
    Serial.println(hosts[i]);
    client.stop();
  }
  return false;
}



void setup() {
  // Turn relay on.
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, HIGH);
  pinMode(LED, OUTPUT);  // inverted, turn off
  digitalWrite(LED, HIGH); 

  Serial.begin(115200);
  EEPROM.begin(512);  //Initialize EEPROM

  int fails = 10; // Try for 5 minutes
  for(;!connectToWifi(30) && fails>0;fails--);
  if (fails==0) {;
    Serial.println("Could not connect to wifi. Reboot!");
    delay(50);
    ESP.restart();
  }
  Serial.println("Have WiFi!");
  
  /* Not needed
  time_t now = time(nullptr);
  Serial.println("Time now:");
  Serial.print(ctime(&now));
  configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
  */
  // May be wait for time to synchronize;

/*
  // First time?
  if (EEPROM.read(0)!=42) {
    EEPROM.write(0,42);
    for (int i=0; i<4; i++) EEPROM.write(i+1,0);
  }
  */

}


unsigned long lastCheck = 0;
unsigned int failedweb = 0;
void loop() {
  if (millis()-lastCheck > 60000L) {
    lastCheck = millis();
    if (WiFi.isConnected()) {
      if (!can_haz_interweb()) {
        failedweb++;
        if (failedweb>10) {  // 10 min no interweb
          // Turn off power to modem
          digitalWrite(RELAY, LOW);
          // Wait 5 secs
          for (int i=0; i<50; i++) {
            delay(100);
            ESP.wdtFeed();
          }
          // Turn power back on by restarting
          ESP.restart();          
        }
      } else {
        failedweb=0;
      }
    } else {  // no wifi
      int fails = 20; // Try for 10 minutes
      for(;!connectToWifi(30) && fails>0;fails--);
      if (fails==0) {
        ESP.restart();
      }
    }
  }
  // REMOVE BEFORE FLIGHT:
  /*
  if (digitalRead(0)==LOW) {
    while(digitalRead(0)==0) {
      ESP.wdtFeed();
      delay(100);
    }
    Serial.println("Program me!");\
    delay(20);
    ESP.rebootIntoUartDownloadMode();
  }
  */

}
