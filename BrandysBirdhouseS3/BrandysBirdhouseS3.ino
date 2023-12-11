/*
   control app https://x.thunkable.com/copy/e8f633a8a9a3e979025112e18446d3b4
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 

   Use this to define multiple segments on the fly or define them as a variable
   // parameters: index, start, stop, mode, color, speed, reverse
  //ws2812fx.setSegment(0,  0,  9, FX_MODE_BLINK, 0xFF0000, 1000, false); // segment 0 is leds 0 - 9
  //ws2812fx.setSegment(1, 10, 19, FX_MODE_SCAN,  0x00FF00, 1000, false); // segment 1 is leds 10 - 19
  //ws2812fx.setSegment(2, 20, 29, FX_MODE_COMET, 0x0000FF, 1000, true);  // segment 2 is leds 20 - 29

  //Implemented
  OTA
  BLE
  2812B LED control
  motor Speed and direction Control
  SD Card


  //Needed
  Audio
  NFC
  LED Control in phone app
*/
#include <stdio.h>
#include <Arduino.h>

//BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#define SERVICE_UUID           "ffbfa26f-69b2-405b-83fc-5708c9f48a30" // UART service UUID
#define CHARACTERISTIC_UUID_RX "ffbfa26f-69b2-405b-83fc-5708c9f48a31"
#define CHARACTERISTIC_UUID_TX "ffbfa26f-69b2-405b-83fc-5708c9f48a32"
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
String rxString = "";    // string to hold input
const char* rxSPEED = "SPD,";
const char* rxBRIGHTNESS = "BRT,";
const char* rxMODE = "MDE,";
const char* rxCOLOR = "RGB,";
const char* txSPEED = "";
const char* txBRIGHTNESS = "";
const char* txMODE = "";
float txValue = 0;

//WiFi
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
const char* ssid = "chlabs_bot";
const char* password = "chlabsrobotseverywhere";
const char deviceName[] = "Brandy's Birdhouse S3";

//Neopixel
#include <WS2812FX.h>
#define LED_PIN    48  // digital pin used to drive the LED strip
#define LED_COUNT 1  // number of LEDs on the strip
int led_BRIGHTNESS = 64;
int led_SPEED = 1000;
int led_MODE = 1;
String led_COLOR = "BLUE";
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

class MyServerCallbacks: public BLEServerCallbacks 
{
    void onConnect(BLEServer* pServer) 
    {
      deviceConnected = true;
      Serial.println("Device Connected");
    };

    void onDisconnect(BLEServer* pServer) 
    {
      deviceConnected = false;
      Serial.println("Device Disonnected");
      pServer->getAdvertising()->start();
      Serial.println("Awaiting client connection...");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks 
{
  void onRead(BLECharacteristic *pCharacteristic) 
  {
    char txString[16]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
    
      pCharacteristic->setValue(txString);
      pCharacteristic->notify();
  }
    void onWrite(BLECharacteristic *pCharacteristic) 
    {
      String rxString = pCharacteristic->getValue().c_str();

      if (rxString.length() > 0) 
      {
        Serial.println("*********");
        Serial.print("\r\nReceived Value: ");

        for (int i = 0; i < rxString.length(); i++) 
        {
          Serial.print(rxString[i]);
        }
         Serial.println();
         
        // Do stuff based on the command received from the app
        if (rxString.indexOf(rxSPEED) != -1) 
        {
          Serial.print("Current LED speed: "); Serial.println(ws2812fx.getSpeed());
          Serial.print("Setting LED speed: ");  Serial.println(rxString.substring(4));
          led_SPEED = rxString.substring(4).toInt();
          led_SPEED = map(led_SPEED, 1, 100, 4975, 50);
          ws2812fx.setSpeed(led_SPEED);
          ws2812fx.service();
          Serial.print("Current LED speed: "); Serial.println(ws2812fx.getSpeed());
        }
        else if (rxString.indexOf(rxBRIGHTNESS) != -1) 
        {
          Serial.print("Current LED Brightness: "); Serial.println(ws2812fx.getBrightness());
          Serial.print("Setting LED brightness: ");  Serial.println(rxString.substring(4));
          led_BRIGHTNESS = rxString.substring(4).toInt();
          Serial.println(led_BRIGHTNESS);
          map(led_BRIGHTNESS, 0, 100, 0, 255)
          ws2812fx.setBrightness(led_BRIGHTNESS);
          ws2812fx.service();
          Serial.print("Current LED Brightness: "); Serial.println(ws2812fx.getBrightness());
        }
        else if (rxString.indexOf(rxMODE) != -1) 
        {
          led_MODE = rxString.substring(4).toInt();
          Serial.print("Current LED mode: "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
          Serial.print("Setting LED Mode: ");  Serial.println(ws2812fx.getModeName(led_MODE));
          ws2812fx.setMode(led_MODE); // segment 0 is leds 0 - 9
          Serial.print("Current LED mode: "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
        }
        else if (rxString.indexOf(rxCOLOR) != -1) 
        {
          const byte numChars = 12;
          char receivedChars[numChars];                   // temporary array for use when parsing
          // variables to hold the parsed data
          int intLedColor = 0;
          
          led_COLOR = rxString.substring(4);
          led_COLOR.toCharArray(receivedChars, 12);
          char * strtokIndx; // this is used by strtok() as an index
          strtokIndx = strtok(receivedChars,",");      // get the first part - the string
          intLedColor = atoi(strtokIndx);     // convert red part to an integer
          int r = intLedColor;
          //  Red value mapped. Now we're looking at the green value in the array
          strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
          intLedColor = atoi(strtokIndx);     // convert this part to an integer
          int g = intLedColor;
          //  Blue's Turn
          strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
          intLedColor = atoi(strtokIndx);     // convert this part to an integer
          int b = intLedColor;
          Serial.print("r: "); Serial.println(r);  Serial.print("g: "); Serial.println(g);  Serial.print("b: ");  Serial.println(b);
          ws2812fx.setColor(r,g,b);
        }
        Serial.println();
        Serial.println("*********");
      }
    }
};

// displays at startup the Sketch running in the Arduino
void display_Running_Sketch (void){
  int cpuSpeed = getCpuFrequencyMhz();
  String the_path = __FILE__;
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc+1);
  int dot_loc = the_cpp_name.lastIndexOf('.');
  String the_sketchname = the_cpp_name.substring(0, dot_loc);

  Serial.print("\nCPU initialized at " + String(cpuSpeed) + "MHz");
  Serial.print("\nBooting ");
  Serial.print(the_sketchname);
  Serial.print("\nCompiled on: ");
  Serial.print(__DATE__);
  Serial.print(" at ");
  Serial.print(__TIME__);
  Serial.print("\nESP-IDF SDK: "); 
  Serial.println(ESP.getSdkVersion());
  Serial.println();
}

void setup() {
  delay(500);
  Serial.setDebugOutput(true);
  Serial.begin(115200);
  delay(500);
  display_Running_Sketch();

  ws2812fx.init();
  ws2812fx.setBrightness(led_BRIGHTNESS);
  // parameters: index, start, stop, mode, color, speed, reverse
  //ws2812fx.setSegment(0,  0,  9, FX_MODE_BLINK, 0xFF0000, 1000, false); // segment 0 is leds 0 - 9
  //ws2812fx.setSegment(1, 10, 19, FX_MODE_SCAN,  0x00FF00, 1000, false); // segment 1 is leds 10 - 19
  //ws2812fx.setSegment(2, 20, 29, FX_MODE_COMET, 0x0000FF, 1000, true);  // segment 2 is leds 20 - 29
  ws2812fx.setSegment(0,  0,  LED_COUNT-1, FX_MODE_STATIC, BLUE, 1000, false); // segment 0 is led 0 a.k.a. Status LED
  ws2812fx.start();
  Serial.println("Neopixels initialized");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("WiFi initialized");

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(deviceName);

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() 
    {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      ws2812fx.setSegment(0,  0,  1, FX_MODE_BREATH, ORANGE, 1000, false); // segment 0 is led 0 a.k.a. Status LED
      ws2812fx.service();
      Serial.println("Start updating " + type);
    })
    .onEnd([]() 
    {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) 
    {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) 
    {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("OTA initialized");

  // Create the BLE Device
  BLEDevice::init(deviceName); // Give it a name
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_RX,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  // Start the service
  pService->start();
  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("BLE initialized");
  
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  Serial.println("\r\nSystem initialized and ready. Awaiting client connection to begin BLE advertising...");
  ws2812fx.service();
}

void loop() {
  if (deviceConnected) 
  {

    /*    This works to send int. Need to change to send string so I can send the mode to the Android app
    // Fabricate some arbitrary junk for now...
    txValue = random(5); // This could be an actual sensor reading!

    // Let's convert the value to a char array:
    char txString[8]; // make sure this is big enuffz
    dtostrf(txValue, 1, 2, txString); // float_val, min_width, digits_after_decimal, char_buffer
    
//    pCharacteristic->setValue(&txValue, 1); // To send the integer value
//    pCharacteristic->setValue("Hello!"); // Sending a test message
    pCharacteristic->setValue(txString);
    
    pCharacteristic->notify(); // Send the value to the app!
    //Serial.print("*** Sent Value: ");
    //Serial.print(txString);
    //Serial.println(" ***");
    */

    // You can add the rxString checks down here instead
    // if you set "rxValue" as a global var at the top!
    // Note you will have to delete "std::string" declaration
    // of "rxValue" in the callback function.
//    if (rxValue.find("A") != -1) { 
//      Serial.println("Turning ON!");
//      digitalWrite(LED, HIGH);
//    }
//    else if (rxValue.find("B") != -1) {
//      Serial.println("Turning OFF!");
//      digitalWrite(LED, LOW);
//    }
  }

//  else restart service and wait 5 minutes for a connection, then restart/deep sleep.
  ws2812fx.service();
  ArduinoOTA.handle();

    // Your normal code to do your task can go here
}
