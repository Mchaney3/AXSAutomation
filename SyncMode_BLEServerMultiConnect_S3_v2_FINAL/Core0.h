#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "c64be9e5-95ed-40f9-b18d-fe7914928ed7"
#define CHARACTERISTIC_UUID "ca06ef51-151a-460f-9e85-23cf4e8aebc8"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      BLEDevice::startAdvertising();
      ws2812fx.setMode(FX_MODE_STATIC);
      ws2812fx.setColor(BLUE);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      ws2812fx.setMode(FX_MODE_BREATH);
      ws2812fx.setColor(ORANGE);
    }
};

void bleTASK( void * pvParameters ){
  // Create the BLE Device
  BLEDevice::init(DeviceName);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Awaiting Client...");
  ws2812fx.setMode(FX_MODE_BREATH);
  ws2812fx.setColor(ORANGE);
  
  for(;;){
    // notify changed value
    if (deviceConnected) {
        static char* ledMode = "1001";
        pCharacteristic->setValue(ledMode);
        pCharacteristic->notify();
        Serial.println(ledMode);
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(200); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    if (value >= 120) value = 0;
  }
}
