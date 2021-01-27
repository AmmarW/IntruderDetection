/*****************************************************************************************
                        Intruder Detection System using IR sensor
                       Semester Project (Instrumentation & Control Lab)
                             Ammar Waheed      [2017-ME-34]
                             M. Abdullah Arif  [2017-ME-05]
                             M. Rizwan Yasin   [2017-ME-11]

  Hardware:
  - Nodemcu esp8266 v3 ch340
  - 16x2 LCD with i2c
  - Sharp IR Sensor GP2Y0A02YK0F
  Pins:
  A0 -- IR analog signal
  D1 -- i2c SCL
  D2 -- i2c SDA
  D3 -- WiFiManager call
  D5 -- Alarm LED/Buzzer
  D6 -- Manual alarm reset
****************************************************************************************/

//Required Libraries (Ensure mentioned versions for correct working)
#include <FS.h>
#include <ArduinoJson.h>          //v5.13.5  
#include <WiFiManager.h>          //Release v2.0.3-apha Development 
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include "IntruderDetection.h"
#include <TimeLib.h>
#include <WidgetRTC.h>

//Global default variables and initializations
int trig = 50;                                      //trigger distance value (20-150cm)
char trig_char[5];                                  //temporary char for storing converted trigger value
char auth[34] = "ehyhjsyGj9mNW4iDLoQXjQlJtvZGXIph"; //Blynk Auth Code sent via Email
int timeout = 120;                                  //trigger pin will start a configPortal AP for 120 seconds then turn it off.
int flag = 0;
int distance = 0;
#define TRIGGER_PIN 0                               // select which pin will trigger the configuration portal (WiFiManager) when set to LOW
IntruderDetection sensor( IntruderDetection::GP2Y0A02YK0F, A0 );  //Ensure correct sensor label and pin
LiquidCrystal_I2C lcd(0x27, 16, 2);
BlynkTimer timer;
WidgetLED led(V2);
WidgetTerminal terminal(V0);
WidgetRTC rtc;
WiFiManager wm;
WiFiManagerParameter custom_field;
WiFiManagerParameter custom_field2;

/***************************************************************/
//Function for reading stored config values
void setupSpiffs()
{
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        lcd.clear();
        lcd.print(" Reading Config ");
        delay(1000);
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          lcd.setCursor(0, 1);
          lcd.print("    Success!   ");
          delay(1000);
          trig = json["trig"];
          strcpy(auth, json["auth"]);

        } else {
          Serial.println("failed to load json config");
          lcd.clear();
          lcd.print("Config Read Fail");
          delay(3000);
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
}

/***************************************************************/
//Blynk Notification function
void notifyOnThings()
{
  int distance = sensor.getDistance();
  if (distance < trig && flag == 0) {
    Blynk.notify("Alert : Intruder Detected");
    // Blynk.email("Subject: Intruder Alert", "Intruder detected !");    //for email notifications using email widget
    String currentTime = String(hour()) + ":" + minute() + ":" + second();
    String currentDate = String(day()) + " " + month() + " " + year();
    terminal.println("Intruder Detected");
    terminal.println(currentDate + " -- " + currentTime);
    terminal.flush();
    flag = 1;
  }
  else if (distance > trig)
  {
    flag = 0;
  }
}
/***************************************************************/
//Alarm function
void alarmsystem()
{
  int distance = sensor.getDistance();
  Blynk.virtualWrite(V5, distance);
  if (distance < trig)
  {
    digitalWrite(D5, HIGH);
    led.on();
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.print("  Intruder!   ");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.print("System  Online");
  }
}
/***************************************************************/
//Manual Alarm reset control function
void alarmreset()
{
  if (digitalRead(D6) == LOW)
  {
    digitalWrite(D5, LOW);
    led.off();
  }
}
/*************************************************************/
//Blynk Alarm reset button control function
BLYNK_WRITE(V1)
{
  int pinData = param.asInt();
  if (pinData == 1)
  {
    digitalWrite(D5, LOW);
    led.off();
  }
}
/*************************************************************/
//WiFi Manager Setup
void WiFi_Manager()
{
  if ( digitalRead(TRIGGER_PIN) == LOW)
  {
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.print("System Offline");
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print("  AP Enabled  ");

    new (&custom_field) WiFiManagerParameter("TriggerDistance", "Trigger Distance(cm)", itoa(trig, trig_char, 10), 3, "placeholder=\"Distance\"");
    new (&custom_field2) WiFiManagerParameter("BlynkAuthToken", "Blynk Auth Token", auth, 34, "placeholder=\"Blynk Auth Token\"");
    wm.addParameter(&custom_field);
    wm.addParameter(&custom_field2);

    wm.setSaveParamsCallback(saveParamCallback);


    std::vector<const char *> menu = {"wifi", "info", "param", "sep", "restart", "exit"};
    wm.setMenu(menu);
    // set dark theme
    wm.setClass("invert");

    // set configportal timeout
    wm.setConfigPortalTimeout(timeout);

    if (!wm.startConfigPortal("ESP8266-AP", "password"))
    {
      Serial.println("failed to connect and hit timeout");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.print("  AP Disabled   ");
      delay(2000);
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.print("  Restarting  ");
      delay(1000);
      ESP.restart();
      delay(3000);
    }
    Serial.println("connected");
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.print("System  Online");
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print("WiFi Connected");
    Blynk.notify("Sytem Back Online!");
  }
}
String getParam(String name) {
  //read parameters
  String value;
  if (wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}
void saveParamCallback() {
  Serial.println("[CALLBACK] saveParamCallback fired");
  Serial.println("PARAM TriggerDistance = " + getParam("TriggerDistance"));
  Serial.println("PARAM BlynkAuthToken = " + getParam("BlynkAuthToken"));


  trig = getParam("TriggerDistance").toInt();

  Serial.println("saving config");
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["trig"] = trig;
  json["auth"] = getParam("BlynkAuthToken");
  lcd.setCursor(0, 1);
  lcd.print("Config Recieved ");
  delay(1000);
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
    lcd.setCursor(0, 1);
    lcd.print("Config Save Fail ");
    delay(3000);
  }
  json.prettyPrintTo(Serial);
  json.printTo(configFile);
  configFile.close();
  lcd.setCursor(0, 1);
  lcd.print("  Config Saved ");
  delay(1000);
  //end save
}

/*************************************************************/
//WiFi Connection Check
BLYNK_CONNECTED()
{
  rtc.begin();
  delay(500);
  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.print("WiFi Connected");
}
/*************************************************************/
void setup()
{
  Serial.begin(115200);
  Serial.println("\n Starting");
  Wire.begin(D2, D1);
  lcd.begin();
  lcd.backlight();
  lcd.print(" UET MECH. DEPT");
  lcd.setCursor(0, 1);
  lcd.print("2017-ME-05,11,34");
  delay(5000);
  setupSpiffs();
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  Blynk.config(auth);
  setSyncInterval(30);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(D5, OUTPUT);
  pinMode(D6, INPUT_PULLUP);
  timer.setInterval(250L, notifyOnThings);
  timer.setInterval(250L, alarmsystem);
  timer.setInterval(250L, alarmreset);
  timer.setInterval(250L, WiFi_Manager);
}
/*************************************************************/
void loop()
{
  Blynk.run();
  timer.run();

  //Blynk Server Connection Check
  if (Blynk.connected() == false)
  {
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.print("Server Offline ");
  }
}
/*************************************************************/
