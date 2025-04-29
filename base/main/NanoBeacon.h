/**
 *  MiniGolf
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: https://www.mikekohn.net/
 * License: BSD
 *
 * Copyright 2025 by Michael Kohn 
 *
 * https://www.mikekohn.net/
 *
 */

#ifndef NANO_BEACON_H
#define NANO_BEACON_H

#include <string.h>

#include "esp_gap_ble_api.h"

class NanoBeacon
{
public:
  NanoBeacon();
  ~NanoBeacon();

  struct Rotation
  {
    Rotation() :
      movement    { 0 },
      no_movement { 0 },
      flags       { 0 },
      clear_flags { false }
    {
      memset(values, 0, sizeof(values));
    }

    int movement;
    int no_movement;
    int flags;
    int values[3];
    bool clear_flags;
  };

  static Rotation rotations[3];

private:
  void init();

  void register_callback();

  static bool is_ibeacon_packet(uint8_t *adv_data, uint8_t adv_data_len);

  static void callback(
    esp_gap_ble_cb_event_t event,
    esp_ble_gap_cb_param_t *param);

  typedef struct
  {
    uint8_t flags[3];
    uint8_t length;
    uint8_t type;
    uint16_t company_id;
    uint16_t beacon_type;
  } __attribute__((packed)) esp_ble_ibeacon_head_t;

  typedef struct
  {
    uint8_t proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t measured_power;
  } __attribute__((packed)) esp_ble_ibeacon_vendor_t;

  typedef struct
  {
    esp_ble_ibeacon_head_t ibeacon_head;
    esp_ble_ibeacon_vendor_t ibeacon_vendor;
  } __attribute__((packed)) esp_ble_ibeacon_t;

  esp_ble_ibeacon_vendor_t vendor_config;

  static const char *TAG;

};

#endif

