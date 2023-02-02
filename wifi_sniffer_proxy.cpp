// vim: set ft=arduino:
/*
   gas_sensor_proxy.cpp
   Copyright (C) 2022 Kryštof Havránek <krystof@havrak.xyz>

   Distributed under terms of the MIT license.
*/

#include "wifi_sniffer_proxy.h"

esp_err_t WiFiSnifferProxy::eventHandler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}

void WiFiSnifferProxy::wifiSnifferPacketHandler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  printf("CHAN=%02d, RSSI=%02d, ADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
  //printf("CHAN=%02d, RSSI=%02d, ADDR1=%02x:%02x:%02x:%02x:%02x:%02x, ADDR2=%02x:%02x:%02x:%02x:%02x:%02x, ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
         ppkt->rx_ctrl.channel,
         ppkt->rx_ctrl.rssi,
         /* ADDR */
         //hdr->addr1[0], hdr->addr1[1], hdr->addr1[2],
         //hdr->addr1[3], hdr->addr1[4], hdr->addr1[5],
         
         hdr->addr2[0], hdr->addr2[1], hdr->addr2[2],
         hdr->addr2[3], hdr->addr2[4], hdr->addr2[5]
         
         //hdr->addr3[0], hdr->addr3[1], hdr->addr3[2],
         //hdr->addr3[3], hdr->addr3[4], hdr->addr3[5]
        );
  WiFiSnifferProxy::getInstance()->logBeaconedDeviceAddress(hdr->addr2);
}


WiFiSnifferProxy* WiFiSnifferProxy::instance = nullptr;

WiFiSnifferProxy::WiFiSnifferProxy()
{
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK( esp_event_loop_init(WiFiSnifferProxy::eventHandler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&WiFiSnifferProxy::wifiSnifferPacketHandler);
  
  for (int i = 0; i < MAX_DEVICE_TRACKED; i++) {
    devices[i].timestampBuffer[1] = 0;
  }
}


WiFiSnifferProxy* WiFiSnifferProxy::getInstance()
{
  if (instance == nullptr) {
    instance = new WiFiSnifferProxy();
  }
  return instance;
}

void WiFiSnifferProxy::switchChannel()
{
  vTaskDelay(WIFI_CHANNEL_SWITCH_INTERVAL / portTICK_PERIOD_MS);
  channel = (channel+1)%WIFI_CHANNEL_MAX;
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}


bool WiFiSnifferProxy::checkForSuspiciousDevices() {
  
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

void WiFiSnifferProxy::logBeaconedDeviceAddress(const uint8_t* address) {
  return;
  
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
    Serial.print("WIFI_SNIFFER_PROXY | logBeaconedDeviceAddress | adding new device, index: ");
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
    Serial.print("WIFI_SNIFFER_PROXY | logBeaconedDeviceAddress | adding new log to device, index ");
    Serial.println(index);
    if (millis() - devices[index].timestampBuffer[devices[index].index] < GAP_SIZE) return;

    if ((devices[index].index + 1) % arraySize == devices[index].start) {
      devices[index].start = (devices[index].start + 1) % arraySize;
    }
    devices[index].index = (devices[index].index + 1) % arraySize;
    devices[index].timestampBuffer[devices[index].index] = millis();
    Serial.print("WIFI_SNIFFER_PROXY | logBeaconedDeviceAddress | log updated ");
  }
}
