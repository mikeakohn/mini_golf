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

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_log.h"

#include "NanoBeacon.h"

NanoBeacon::NanoBeacon()
{
  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_bt_controller_init(&bt_cfg);
  esp_bt_controller_enable(ESP_BT_MODE_BLE);

  init();

  esp_ble_scan_params_t ble_scan_params =
  {
    .scan_type          = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval      = 0x50,
    .scan_window        = 0x30,
    .scan_duplicate     = BLE_SCAN_DUPLICATE_DISABLE
  };

  esp_ble_gap_set_scan_params(&ble_scan_params);
}

NanoBeacon::~NanoBeacon()
{
}

void NanoBeacon::init()
{
  esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
  esp_bluedroid_init_with_cfg(&bluedroid_cfg);
  esp_bluedroid_enable();

  register_callback();
}

void NanoBeacon::register_callback()
{
  ESP_LOGI(TAG, "Register callback.");

  // Register the scan callback function to the gap module.
  esp_err_t status = esp_ble_gap_register_callback(callback);

  if (status != ESP_OK)
  {
    ESP_LOGE(TAG, "Gap register error: %s", esp_err_to_name(status));
    return;
  }
}

bool NanoBeacon::is_ibeacon_packet(uint8_t *adv_data, uint8_t adv_data_len)
{
  bool result = false;

  // For iBeacon packet format, please refer to Apple "Proximity Beacon
  // Specification" doc. Constant part of iBeacon data>
  esp_ble_ibeacon_head_t ibeacon_common_head =
  {
    .flags = { 0x02, 0x01, 0x06 },
    .length = 0x1a,
    .type = 0xff,
    .company_id = 0x004c,
    .beacon_type = 0x1502
  };

  if (adv_data != NULL && adv_data_len == 0x1e)
  {
    result = memcmp(adv_data, (uint8_t *)&ibeacon_common_head, sizeof(ibeacon_common_head)) != 0;

#if 0
    if (!memcmp(adv_data, (uint8_t *)&ibeacon_common_head, sizeof(ibeacon_common_head)))
    {
      result = true;
    }
#endif
  }

  return result;
}

NanoBeacon::Rotation NanoBeacon::rotations[3];

void NanoBeacon::callback(
  esp_gap_ble_cb_event_t event,
  esp_ble_gap_cb_param_t *param)
{
  esp_err_t err;

  switch (event)
  {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    {
      // This is for a sender.
      break;
    }
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    {
      // The unit of the duration is second, 0 means scan permanently.
      uint32_t duration = 0;
      esp_ble_gap_start_scanning(duration);

      break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
    {
      // Scan start complete event to indicate scan start successfully or
      // failed.
      err = param->scan_start_cmpl.status;

      if (err != ESP_BT_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
      }

      break;
    }
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    {
      // Adv start complete event to indicate adv start successfully or failed.
      err = param->adv_start_cmpl.status;

      if (err != ESP_BT_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "Adv start failed: %s", esp_err_to_name(err));
      }

      break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
      esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;

      switch (scan_result->scan_rst.search_evt)
      {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
        {
          // Search for BLE iBeacon Packet.
          uint8_t *adv_data = scan_result->scan_rst.ble_adv;
          uint8_t adv_data_len = scan_result->scan_rst.adv_data_len;

          // Original code was to detect iBeacons.
          if (is_ibeacon_packet(adv_data, adv_data_len))
          {
#if 0
            esp_ble_ibeacon_t *ibeacon_data =
              (esp_ble_ibeacon_t *)(scan_result->scan_rst.ble_adv);

            ESP_LOGI(TAG, "---------- iBeacon Found ----------");

            esp_log_buffer_hex("IBEACON_DEMO: Device address:",
              scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );

            esp_ble_ibeacon_vendor_t &vendor = ibeacon_data->ibeacon_vendor;

            esp_log_buffer_hex("IBEACON_DEMO: Proximity UUID:",
              vendor.proximity_uuid, ESP_UUID_LEN_128);

            #define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00)>>8) + (((x)&0xFF)<<8))

            uint16_t major = ENDIAN_CHANGE_U16(vendor.major);
            uint16_t minor = ENDIAN_CHANGE_U16(vendor.minor);

            ESP_LOGI(TAG, "Major: 0x%04x (%d)", major, major);
            ESP_LOGI(TAG, "Minor: 0x%04x (%d)", minor, minor);
            ESP_LOGI(TAG, "Measured power (RSSI at a 1m distance):%d dbm",
              vendor.measured_power);
            ESP_LOGI(TAG, "RSSI of packet:%d dbm",
              scan_result->scan_rst.rssi);
#endif
          }
            else
          {
            char text[256];
            char hex[4];

            // Look for a NanoBeacon with this made up address. The last
            // byte is the unique ID for the 3 beacons being used for this
            // project.
            const uint8_t address[] = { 0x06, 0x05, 0x04, 0x03, 0x02, 0x01 };
            bool use_data = false;
            int index = 0;

            if (ESP_BD_ADDR_LEN == 6)
            {
              if (memcmp(address, scan_result->scan_rst.bda, 5) == 0)
              {
                use_data = true;
                index = scan_result->scan_rst.bda[5] - 1;
                if (index < 0 || index > 2) { index = 0; }
              }
            }

            if (! use_data) { break; }

            esp_log_buffer_hex("Address:",
              scan_result->scan_rst.bda, ESP_BD_ADDR_LEN );

            uint8_t *adv_data = scan_result->scan_rst.ble_adv;
            uint8_t adv_data_len = scan_result->scan_rst.adv_data_len;

            text[0] = 0;

            for (int i = 0; i < adv_data_len; i++)
            {
              sprintf(hex, "%02x ", adv_data[i]);
              strcat(text, hex);
            }

            ESP_LOGI(TAG, "%s", text);

            if (adv_data[0] == 0xff && adv_data[1] == 0xff)
            {
              int v0 = adv_data[2] | (adv_data[3] << 8);
              int v1 = adv_data[4] | (adv_data[5] << 8);
              int v2 = adv_data[6] | (adv_data[7] << 8);

              Rotation &rotation = rotations[index];

              if (rotation.values[0] == 0)
              {
                rotation.values[0] = v0;
                rotation.values[1] = v1;
                rotation.values[2] = v2;
              }

              int d0 = rotation.values[0] - v0;
              int d1 = rotation.values[1] - v1;
              int d2 = rotation.values[2] - v2;

              if (d0 < 0) { d0 = -d0; }
              if (d1 < 0) { d1 = -d1; }
              if (d2 < 0) { d2 = -d2; }

              int changes = 0;

              if (d0 > 2) { changes += 1; }
              if (d1 > 2) { changes += 1; }
              if (d2 > 2) { changes += 1; }

              if (changes >= 1)
              {
                rotation.values[0] = v0;
                rotation.values[1] = v1;
                rotation.values[2] = v2;

                rotation.movement += 1;
                rotation.no_movement = 0;

                ESP_LOGI(TAG, "MOVED %d %d / %d %d",
                  rotation.movement,
                  rotation.no_movement,
                  rotation.flags,
                  rotation.clear_flags);
              }
                else
              {
                rotation.no_movement += 1;

                if (rotation.movement != 0 && rotation.no_movement > 1)
                {
                  rotation.movement = 0;
                  rotation.flags = 1;
                }
              }
            }
          }

          break;
        }
        default:
        {
          break;
        }
      }

      for (int i = 0; i < 3; i++)
      {
        if (rotations[i].clear_flags)
        {
          rotations[i].clear_flags = false;
          rotations[i].flags       = 0;
        }
      }

      break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
    {
      if ((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "Scan stop failed: %s", esp_err_to_name(err));
      }
        else
      {
        ESP_LOGI(TAG, "Stop scan successfully");
      }

      break;
    }
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    {
      if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
      {
        ESP_LOGE(TAG, "Adv stop failed: %s", esp_err_to_name(err));
      }
        else
      {
        ESP_LOGI(TAG, "Stop adv successfully");
      }

      break;
    }
    default:
    {
        break;
    }
  }
}

const char *NanoBeacon::TAG = "NANO";

