// vim: set ft=arduino:
/*
 * gas_sensor_proxy.h
 * Copyright (C) 2022 Kryštof Havránek <krystof@havrak.xyz>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef BLUETOOTH_SNIFFER_PROXY_H
#define BLUETOOTH_SNIFFER_PROXY_H

#include <Arduino.h>
#include "BluetoothSerial.h"
#include "BTScan.h"
#include "log_config.h"

static BluetoothSerial SerialBT;

class BluetoothSnifferProxy{
  private:
    static BluetoothSnifferProxy* instance;

    bool active = true;
    bool error = false;
    deviceLog devices[MAX_DEVICE_TRACKED];

    BluetoothSnifferProxy();

    uint64_t timeOfMeasurement;

    bool first = true;

		


  public:

    /**
     * main method to access BluetoothSnifferProxy
     * if instance of proxy wasn't yet created
     * method will do the initialization
     */
    static BluetoothSnifferProxy* getInstance();

    void scan();

    void logBeaconedDeviceAddress(const uint8_t* address);

    bool checkForSuspiciousDevices();

    /**
     * returns whether periphery is active
     *
     * @return bool - periphery state
     */
    bool isActive(){
      return active;
    }

    /**
     * activates or deactivates periphery
     * when set to false sensor data will not be read
     *
     * @param bool active
     */
    void setActive(bool active){
      this->active = active;
    }


};

#endif /* !BLUETOOTH_SNIFFER__PROXY_H */
