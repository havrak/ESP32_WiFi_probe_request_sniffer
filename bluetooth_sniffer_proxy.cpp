// vim: set ft=arduino:
/*
   gas_sensor_proxy.cpp
   Copyright (C) 2022 Kryštof Havránek <krystof@havrak.xyz>

   Distributed under terms of the MIT license.
*/

#include "bluetooth_sniffer_proxy.h"



BluetoothSnifferProxy* BluetoothSnifferProxy::instance = nullptr;

BluetoothSnifferProxy::BluetoothSnifferProxy()
{
  SerialBT.begin("ESP32test"); //Bluetooth device name
  for (int i = 0; i < MAX_DEVICE_TRACKED; i++) {
    devices[i].timestampBuffer[1] = 0;
  }
}


BluetoothSnifferProxy* BluetoothSnifferProxy::getInstance()
{
  if (instance == nullptr) {
    instance = new BluetoothSnifferProxy();
  }
  return instance;
}

void BluetoothSnifferProxy::scan()
{
  BTScanResults* discovered = SerialBT.discover(6000);
  Serial.print("BLUETOOTH_SNIFFER_PROXY | scan | Count discovered: ");
  Serial.println(discovered->getCount());
  uint8_t addr[6];
  for(int i = 0; i < discovered->getCount(); i++){
    esp_bd_addr_t* btAddr = discovered->getDevice(i)->getAddress().getNative();
    addr[0] = *btAddr[0];
    addr[1] = *btAddr[1];
    addr[2] = *btAddr[2];
    addr[3] = *btAddr[3];
    addr[4] = *btAddr[4];
    addr[5] = *btAddr[5];
    printf("BLUETOOTH_SNIFFER_PROXY | scan | ADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
         addr[0], addr[1], addr[2],
         addr[3], addr[4], addr[5]);
    BluetoothSnifferProxy::getInstance()->logBeaconedDeviceAddress(addr); 
  }
}


bool BluetoothSnifferProxy::checkForSuspiciousDevices() {
 bool toReturn = false;
  for (int i = 0; i < MAX_DEVICE_TRACKED; i++) {
    if (devices[i].timestampBuffer[1] == 0) continue; // log not occupied
    if (millis() - devices[i].timestampBuffer[devices[i].index] > FORGET_TIME) {
      devices[i].index = 1;
      devices[i].start = 0;
      devices[i].timestampBuffer[1] = 0;
      continue;
    } // device not seen for a while
    uint16_t counter = 0;
    uint16_t endIndex = (devices[i].index+1)%arraySize; // on index is last item we have written to
    for(int j = (devices[i].start+1)%arraySize; j!= endIndex; j=(j+1)%arraySize){
      if(millis()-devices[i].timestampBuffer[j]<LOGGED_TIME) counter++;
    }
    if(counter >=neededNumberOfLogs){
      if(devices[i].alertRaised < 200)
      devices[i].alertRaised++;
      if(devices[i].alertRaised < DEVICE_IGNORED_AFTER_ALARMS) 
        toReturn = true; // some non ignored devices has been here for longer that 10 minutes
    }
  }
  return toReturn;
}

void BluetoothSnifferProxy::logBeaconedDeviceAddress(const uint8_t* address) {

  int index = -1;
  for (int i = 0; i < MAX_DEVICE_TRACKED; i++) {
    if (devices[i].mac[0] == address[0] && devices[i].mac[1] == address[1] && devices[i].mac[2] == address[2] && devices[i].mac[3] == address[3] && devices[i].mac[4] == address[4] && devices[i].mac[5] == address[5]) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    index = 0;
    uint64_t minTime = devices[0].timestampBuffer[devices[0].index];
    for (int i = 1; i < MAX_DEVICE_TRACKED; i++) {
      if (minTime > devices[i].timestampBuffer[devices[i].index]) {
        index = i;
        minTime = devices[i].timestampBuffer[devices[i].index];
      }
    }
    Serial.print("BLUETOOTH_SNIFFER_PROXY | logBeaconedDeviceAddress | adding new device, index: ");
    Serial.println(index);
    devices[index].mac[0] = address[0];
    devices[index].mac[1] = address[1];
    devices[index].mac[2] = address[2];
    devices[index].mac[3] = address[3];
    devices[index].mac[4] = address[4];
    devices[index].mac[5] = address[5];
    devices[index].index = 1;
    devices[index].start = 0;
    devices[index].timestampBuffer[1] = millis();

  } else {
    Serial.print("BLUETOOTH_SNIFFER_PROXY | logBeaconedDeviceAddress | adding new log to device, index ");
    Serial.println(index);
    if (millis() - devices[index].timestampBuffer[devices[index].index] < GAP_SIZE) return;

    if (devices[index].index + 1 % arraySize == devices[index].start) {
      devices[index].start++;
    }
    devices[index].index = devices[index].index + 1 % arraySize;
    devices[index].timestampBuffer[devices[index].index] = millis();
    Serial.print("BLUETOOTH_SNIFFER_PROXY | logBeaconedDeviceAddress | log updated ");
  }
}
