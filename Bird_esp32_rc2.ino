///////////////////////////////////////////////////////////
//  Connections:                                         //
//  ESP                   - Scooter                      //
//  PIN_SWITCH_1 (GPIO13) - BLUE                         //
//  DEFAULT TXD2 (GPIO17) - GREEN                        //
//  PIN_LED (GPIO15)      - SOME LED FOR FUN             //
//  BUZZER_PIN (GPIO26)   - SOME PIEZO SPEAKER FOR FUN   //
///////////////////////////////////////////////////////////

//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY connection settings 
#define REMOTEXY_BLUETOOTH_NAME "Scooter_beta"

// OTA Wifi Settings
const char* ssid = "SSID";
const char* password = "PASSWORD";

// BMS Power on pin
#define PIN_SWITCH_1 13

// LED pin
#define PIN_LED 15

// Piezo speaker pin
#define BUZZER_PIN 26

// RemoteXY select connection mode and include library 
#define REMOTEXY_MODE__ESP32CORE_BLE

#include <RemoteXY.h>
#include <NonBlockingRtttl.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

// Sample RTTTL music xD
const char * mario = "mario:d=4,o=5,b=100:16e6,16e6,32p,8e6,16c6,8e6,8g6,8p,8g,8p";
//const char * starwars = "star_wars:d=16,o=5,b=100:4e,4e,4e,8c,p,g,4e,8c,p,g,4e,4p,4b,4b,4b,8c6,p,g,4d#,8c,p,g,4e,8p";
//const char * poweron = "poweron:d=32,o=5,b=100:c,c#,d#,e,f#,g#,a#,b";
//const char * poweroff = "poweroff:d=32,o=5,b=100:b,a#,g#,f#,e,d#,c#,c";

// RemoteXY configurate  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =   // 62 bytes
  { 255,2,0,13,0,55,0,16,0,1,3,3,21,41,8,21,2,26,2,1,
  20,16,22,11,2,26,31,31,79,78,0,79,70,70,0,70,16,11,83,9,
  9,26,37,0,70,16,40,83,9,9,26,6,0,67,4,31,49,20,5,2,
  26,11 };
  
// this structure defines all the variables and events of your control interface 
struct {

    // input variables
  uint8_t select_1; // =0 if select position A, =1 if position B, =2 if position C, ... 
  uint8_t switch_1; // =1 if switch ON and =0 if OFF 

    // output variables
  uint8_t led_1_r; // led state 0 .. 1 
  uint8_t led_2_b; // led state 0 .. 1 
  char text_1[11];  // string UTF8 end zero

    // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

////////////////////////////////////////////
//         Scooter Starter include        //
///////////////////////////////////////////
#define INTERVAL 1000
unsigned long start_time = 0;
int run_once = 0;

byte messageOff[] = {0xA6, 0x12, 0x02, 0x10, 0x14, 0xCF}; //If the scooter is on turn it off.
byte messageB[] = {0xA6, 0x12, 0x02, 0x11, 0x14, 0x0B}; //Not sure what this does?
byte messageStart[] = {0xA6, 0x12, 0x02, 0x15, 0x14, 0x30}; //This is the unlock code. 27km/h
byte messageLimit[] = {0xA6, 0x12, 0x02, 0x35, 0xFF, 0x38}; //This is the unlock code. 21km/h
byte messageWalk[] = {0xA6, 0x12, 0x02, 0XF5, 0x14, 0x45}; //This is the unlock code. 4km/h

////////////////////////////////////////////
//         Scooter Command include        //
///////////////////////////////////////////

void setup() 
{
  RemoteXY_Init (); 
  
  Serial.begin(9600);
  Serial2.begin(9600);

  pinMode (PIN_SWITCH_1, OUTPUT);
  pinMode (BUZZER_PIN, OUTPUT);
  pinMode (PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, HIGH);

  Serial2.write(messageOff, sizeof(messageOff));
  delay(500);
  Serial2.write(messageStart, sizeof(messageStart));


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  ArduinoOTA.setHostname(REMOTEXY_BLUETOOTH_NAME);
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  piezo(mario);
}

void loop() 
{ 
  RemoteXY_Handler ();
  
  digitalWrite(PIN_SWITCH_1, HIGH);

   if(RemoteXY.switch_1 == 1){
    if(millis() > start_time + INTERVAL){
        start_time = millis();
        switch (RemoteXY.select_1) {
          case 0:
          Serial2.write(messageStart, sizeof(messageStart));
          sprintf (RemoteXY.text_1, "27km/h");
          break;
          case 1:
          Serial2.write(messageLimit, sizeof(messageLimit));
          sprintf (RemoteXY.text_1, "20km/h");
          break;
          case 2:
          Serial2.write(messageWalk, sizeof(messageWalk));
          sprintf (RemoteXY.text_1, "4km/h");
          break;
        }
        run_once = 0;
      }
    RemoteXY.led_1_r = 1;
    }
    else{
      RemoteXY.led_1_r = 0;
      if (run_once == 0){
      Serial2.write(messageOff, sizeof(messageOff));
      sprintf (RemoteXY.text_1, "LOCKED");
      run_once = 1;
      }
    }

  if(RemoteXY.connect_flag == 1){ //BLE connection indication for the smartphone;
    RemoteXY.led_2_b = 1;
    }
    else{
      RemoteXY.led_2_b = 0;
    }

ArduinoOTA.handle();
}