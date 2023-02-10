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


	wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
	wifi_ieee80211_mgmt_packet_t *ipkt = (wifi_ieee80211_mgmt_packet_t *)ppkt->payload;
	wifi_ieee80211_probe_reqest_body_parsed_t ipktBody;

	uint16_t fc = ntohs(ipkt->hdr.frame_ctrl);

	if((fc & 0xFF00) != 0x4000){
		return;
	}

	if(fc & 0x0001){
		printf("HT FIELD PRESENT");
	}

	printf("CHAN=%02d, RSSI=%02d, LEN=%04d, ADDR=%02x:%02x:%02x:%02x:%02x:%02x\n",
			ppkt->rx_ctrl.channel,
			ppkt->rx_ctrl.rssi,
			ppkt->rx_ctrl.sig_len,

			ipkt->hdr.send_addr[0], ipkt->hdr.send_addr[1], ipkt->hdr.send_addr[2],
			ipkt->hdr.send_addr[3], ipkt->hdr.send_addr[4], ipkt->hdr.send_addr[5]

			);
	char ssid[SSID_MAX_LEN] = "\0";
	uint16_t maxIndex = ppkt->rx_ctrl.sig_len-9;
	DOT11_FRAME_BODY_PARSING_DEBUG{
		printf("\tBODY:");
		for(uint16_t i = 0; i < maxIndex; i++){
			printf("%02X ", ipkt->body[i]);
		}
		printf("\n");
	}

	for(uint16_t i = 0; i < maxIndex; i++){
		DOT11_FRAME_BODY_PARSING_DEBUG printf("Element: %d, val: %d\n", i, ipkt->body[i]);
		switch(ipkt->body[i]){
			case DOT11_FRAME_BODY_ELEMENT_ID_SSID:
				{
					uint8_t ssidLenght = ipkt->body[++i];
					for(uint8_t j =0; j < ssidLenght; ++j){
						ssid[j] = ipkt->body[++i];
					}
					DOT11_FRAME_BODY_PARSING_DEBUG printf("\tSSID: %s\n", ssid);
					break;
				}
			case DOT11_FRAME_BODY_ELEMENT_ID_DSSS:
				ipktBody.dsss_channel = ipkt->body[i+2];
				i +=2;
				DOT11_FRAME_BODY_PARSING_DEBUG printf("\tDSSS: %d\n",ipktBody.dsss_channel);
				break;
			case DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_RATES:
				{
					uint8_t noOfSupportedRates = ipkt->body[++i];
					DOT11_FRAME_BODY_PARSING_DEBUG printf("\tSUPPORTED RATES: ");
					for(uint8_t j = 0; j < noOfSupportedRates; ++j){
						ipktBody.supported_rates[j] = ipkt->body[++i];
						DOT11_FRAME_BODY_PARSING_DEBUG printf("%02X ", ipktBody.supported_rates[j]);
					}
					DOT11_FRAME_BODY_PARSING_DEBUG printf("\n");
					break;
				}
			case DOT11_FRAME_BODY_ELEMENT_ID_HT_CAPABILITIES:
				i++; // skip length
				ipktBody.ht_capability_info = (((uint16_t) ipkt->body[++i]) << 8) + ipkt->body[++i];
				ipktBody.ht_a_mpdu_param = ipkt->body[++i];
				for(uint8_t j = 0; j < 16; ++j)
					ipktBody.supported_mcs_set[j] = ipkt->body[++i];
				ipktBody.ht_extended_cap = (((uint16_t) ipkt->body[++i]) << 8) + ipkt->body[++i];
				ipktBody.ht_transmit_beamforming_cap = (((uint32_t) ipkt->body[++i]) << 8) + (((uint32_t) ipkt->body[++i]) << 8) + (((uint32_t) ipkt->body[++i]) << 8) + (((uint32_t) ipkt->body[++i]) << 8);
				ipktBody.ht_asel_cap = ipkt->body[++i];
				DOT11_FRAME_BODY_PARSING_DEBUG printf("\tHT_CAP: read\n");
				break;
			case DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_RATES_EXTENDED: // not a reliable field
				i += ipkt->body[++i]; // skip whole field
				DOT11_FRAME_BODY_PARSING_DEBUG printf("\tSUPPORTED RATES EXT\n");
				break;
			case DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_OPERATING_CLASSES: // not a reliable field
				i += ipkt->body[++i]; // skip whole field
				DOT11_FRAME_BODY_PARSING_DEBUG printf("\tSUPPORTED OPERATING CLASSES\n");
				break;
			case DOT11_FRAME_BODY_ELEMENT_ID_2040COEX:
				ipktBody.coex_20_40 = ipkt->body[i+2];
				DOT11_FRAME_BODY_PARSING_DEBUG printf("\t20/40 COEX: %d\n",ipktBody.coex_20_40);
				i +=2;
				break;
			case DOT11_FRAME_BODY_ELEMENT_ID_EXTENDED_CAPABILITES:
				{
					uint8_t noOfFields = ipkt->body[++i];
					for(uint8_t j = 0; j < 11; ++j){
						if(j < noOfFields)
							ipktBody.extended_cap[j] = ipkt->body[++i];
						else
							ipktBody.extended_cap[j] = 0;
					}
					DOT11_FRAME_BODY_PARSING_DEBUG printf("\tEXTENDED CAP: read\n");
					break;
				}
			default:
				if(ipkt->body[i] == 221) i = maxIndex; // vendor specific field is kinda strange
				i += ipkt->body[++i]; // skip whole field
		}
	}

	bool updated = false;
	for(uint16_t i =0; i < MAX_DEVICE_TRACKED; i++){
		if(compareByteArray(instance->burstFrames[i].addr, ipkt->hdr.send_addr,6)){
			updated = true;
			instance->burstFrames[i].innerBurstSpace[instance->burstFrames[i].burstFrameSpaceIndex] = esp_timer_get_time()-instance->burstFrames[i].lastFrame;
			instance->burstFrames[i].lastFrame =  esp_timer_get_time();
			instance->burstFrames[i].burstFrameSpaceIndex = (instance->burstFrames[i].burstFrameSpaceIndex+1)%20;
			mergeFrameBodyParsed(&(instance->burstFrames[i].body), &ipktBody); // update
		}
	}
	if(!updated){
		uint8_t addr[6] = {0};
		uint16_t i =0;
		for(; i < MAX_DEVICE_TRACKED; i++)
			if(compareByteArray(instance->burstFrames[i].addr, addr,6))
				break;
		if(!compareByteArray(instance->burstFrames[i].addr, addr,6)){ // TODO: didn't found empty space

		}
		memcpy((instance->burstFrames[i].addr), ipkt->hdr.send_addr, 6);
		memcpy(&(instance->burstFrames[i].body), &ipktBody, sizeof(ipktBody));
		instance->burstFrames[i].lastFrame =  esp_timer_get_time();
		instance->burstFrames[i].burstFrameSpaceIndex = 0;
		printf("NEW MAC FOUND\n");
	}
}

bool WiFiSnifferProxy::compareByteArray(uint8_t *arr1, uint8_t *arr2, uint16_t len){
	for(uint8_t i = 0; i < len; i++){
		if(arr1[i]!= arr2[i]) return false;
	}
	return true;
}

void WiFiSnifferProxy::mergeFrameBodyParsed(wifi_ieee80211_probe_reqest_body_parsed_t *dest, wifi_ieee80211_probe_reqest_body_parsed_t *toMerge){
	if(dest->dsss_channel == 0) dest->dsss_channel = toMerge->dsss_channel;
	if(dest->ht_capability_info == 0)  dest->ht_capability_info = toMerge->ht_capability_info;
	if(dest->ht_a_mpdu_param == 0 ) dest->ht_a_mpdu_param = toMerge->ht_a_mpdu_param;
	if(dest->ht_extended_cap == 0) dest->ht_extended_cap = toMerge->ht_extended_cap;
	if(dest->ht_transmit_beamforming_cap == 0) dest->ht_transmit_beamforming_cap = toMerge->ht_transmit_beamforming_cap;
	if(dest->coex_20_40 == 0) dest->coex_20_40 = toMerge->coex_20_40;
	if(dest->ht_asel_cap == 0) dest->ht_asel_cap = toMerge->ht_asel_cap;
	int8_t i = 0;
	for(; i < 8; i++) if(dest->supported_rates[i] == 0 && toMerge->supported_rates[i] !=0) i=255;
	if(i ==255) memcpy(dest->supported_rates, toMerge->supported_rates, 8);
	i =0;
	for(; i < 16; i++) if(dest->supported_mcs_set[i] == 0 && toMerge->supported_mcs_set[i] !=0) i=255;
	if(i ==255) memcpy(dest->supported_mcs_set, toMerge->supported_mcs_set, 8);
	i =0;
	for(; i < 11; i++) if(dest->extended_cap[i] == 0 && toMerge->extended_cap[i] !=0) i=255;
	if(i ==255) memcpy(dest->extended_cap, toMerge->extended_cap, 8);


};

void WiFiSnifferProxy::updateDeviceLog(){
	static const char *key = "anakda0251ms";
	uint8_t hmacResult[32];
	mbedtls_md_context_t ctx;
	mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

	for(uint8_t i  = 0; i < MAX_DEVICE_TRACKED; i++){
		if(esp_timer_get_time() - burstFrames[i].lastFrame > BURST_DELAY*1000 && !compareByteArray(burstFrames[i].addr,emptyMAC, 6)){
			mbedtls_md_init(&ctx);
			mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
			mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, strlen(key));
			mbedtls_md_hmac_update(&ctx, (const unsigned char *) &(burstFrames[i].body), sizeof(wifi_ieee80211_probe_reqest_body_parsed_t));
			mbedtls_md_hmac_finish(&ctx, hmacResult);
			mbedtls_md_free(&ctx);

			printf("PROCESSING ADDR=%02x:%02x:%02x:%02x:%02x:%02x \n",

					burstFrames[i].addr[0], burstFrames[i].addr[1], burstFrames[i].addr[2],
					burstFrames[i].addr[3], burstFrames[i].addr[4], burstFrames[i].addr[5]

					);

			// MAC
			bool done = false;
			for(uint8_t j = 0; j < MAX_DEVICE_TRACKED; j++)
				if(compareByteArray(devices[j].lastUnderMAC, burstFrames[i].addr,6)){
					devices[j].lastSeen = burstFrames[i].lastFrame;
					printf("\tLOG UPDATED BY MAC\n");
					done = true;
					memset(&burstFrames[i],0, sizeof(burstFrames[i]));
					break;
				}
			/* // signature */
			if(done) continue;
			 for(uint8_t j = 0; j < MAX_DEVICE_TRACKED; j++)
				if(compareByteArray(hmacResult, devices[j].signature, 16)){
					printf("\tLAST SEEN UNDER ADDR=%02x:%02x:%02x:%02x:%02x:%02x \n",

					devices[j].lastUnderMAC[0], devices[j].lastUnderMAC[1], devices[j].lastUnderMAC[2],
					devices[j].lastUnderMAC[3], devices[j].lastUnderMAC[4], devices[j].lastUnderMAC[5]

					);
					memcpy(devices[j].lastUnderMAC, burstFrames[i].addr, 6);
					devices[j].lastSeen = burstFrames[i].lastFrame;
					uint32_t sum= 0;
					for(uint8_t q = 0; q < burstFrames[i].burstFrameSpaceIndex; q ++ ){
						sum +=burstFrames[i].innerBurstSpace[q];
					}
					devices[j].innerBurstDelay += sum/(burstFrames[i].burstFrameSpaceIndex+1);
					devices[j].innerBurstDelay /=2;
					devices[j].appearedUnderNoMAC++;
					printf("\tLOG UPDATED BY SIGNATURE\n");
					done = true;
					memset(&burstFrames[i],0, sizeof(burstFrames[i]));
					break;
				}
			if(done) continue;
			/* // TODO: time difference */
			 // new device
			 for(uint8_t j = 0; j < MAX_DEVICE_TRACKED; j++){
				if(compareByteArray(devices[j].lastUnderMAC, emptyMAC,6)){
					memcpy(devices[j].signature, hmacResult, 16);
					memcpy(devices[j].lastUnderMAC, burstFrames[i].addr, 6);
					devices[j].lastSeen = burstFrames[i].lastFrame;
					uint32_t sum= 0;
					for(uint8_t q = 0; q < burstFrames[i].burstFrameSpaceIndex; q ++ ){
						sum +=burstFrames[i].innerBurstSpace[q];
					}
					devices[j].innerBurstDelay = sum/(burstFrames[i].burstFrameSpaceIndex+1);
					printf("LOG HAD TO BE ADDED\n");
					done = true;
					memset(&burstFrames[i],0, sizeof(burstFrames[i]));
					break;
				}
			 }
		}

	}
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

	const wifi_promiscuous_filter_t filt = {
		.filter_mask = WIFI_EVENT_MASK_AP_PROBEREQRECVED
	};
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filt)); //set filter mask esp_wifi_set_promiscuous(true);
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&WiFiSnifferProxy::wifiSnifferPacketHandler));
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); //set 'true' the promiscuous mode


	for (int i = 0; i < MAX_DEVICE_TRACKED; i++) {
		memset(&burstFrames[i], 0, sizeof(burstFrames[i]));
		memset(&devices[i], 0, sizeof(devices[i]));

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
	channel = ((channel+1)%3);
	esp_wifi_set_channel(WiFiChannels[channel], WIFI_SECOND_CHAN_NONE);
}


