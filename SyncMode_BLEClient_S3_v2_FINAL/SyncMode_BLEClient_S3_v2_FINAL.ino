#include <stdio.h>
#include <string.h>
#include <WS2812FX.h>
#include "soc/soc.h" //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems

#define LED_COUNT 1
#define LED_PIN 48
WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

#include "Core0.h"
TaskHandle_t bleClient;

void setup() {
  int cpuSpeed = getCpuFrequencyMhz();
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print("ESP32 SDK: ");
  Serial.println(ESP.getSdkVersion());
  Serial.println(F("START " __FILE__ " from " __DATE__));
  Serial.println("CPU running at " + String(cpuSpeed) + "MHz");
  Serial.println("");
  
  xTaskCreatePinnedToCore(
                    bleTASK,   /* Task function. */
                    "bleClient",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    0,           /* Always highest priority to assure audio quality */
                    &bleClient,      /* Task handle to keep track of created task */
                    0);          /* pin Bluetooth to core 0 */                  
  delay(500);

  ws2812fx.init();
  ws2812fx.setSpeed(1000);
  ws2812fx.setBrightness(8);
  ws2812fx.setMode(FX_MODE_STATIC);
  ws2812fx.setColor(GREEN);
  ws2812fx.start();
}


void loop() {
  ws2812fx.service();
}
