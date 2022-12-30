// vim: set ft=arduino:
/*
 * log_config.h
 * Copyright (C) 2022 Kryštof Havránek <krystof@havrak.xyz>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef LOG_CONFIG_H
#define LOG_CONFIG_H

#define MAX_DEVICE_TRACKED 40


#define GAP_SIZE (10000) // log every 10 seconds
#define LOGGED_TIME (10*60000) // 10 minutes
#define FORGET_TIME (LOGGED_TIME/2)
#define TOLERANCE 0.8  // 80% of logs have to fall in LOGGED_TIME (that is 80% of logs have to be younger than 10 minutes)

static const uint16_t arraySize = LOGGED_TIME/GAP_SIZE+10; // + add buffer space
static const uint16_t neededNumberOfLogs = LOGGED_TIME/GAP_SIZE/5*4;

typedef struct{
  uint8_t mac[6];
  uint64_t timestampBuffer[arraySize]; // switch to
  uint8_t index = 1;
  uint8_t start = 0;
} deviceLog;

#endif /* !LOG_CONFIG_H */
