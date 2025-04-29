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

#include "PN532.h"

// These will get computed before being sent out, so default them to 0.
#define _lcs 0
#define _dcs 0

const uint8_t PN532::packet_sam_config[] =
{
  0x00, 0x00, 0xff,
     5, _lcs,
  0xd4, PN532_CMD_SAM_CONFIGURATION, 0x01, 0x14, 0x01,
  _dcs, 0x00
};

const uint8_t PN532::packet_get_firmware_version[] =
{
  0x00, 0x00, 0xff,
     2, _lcs,
  0xd4, PN532_CMD_GET_FIRMWARE_VERSION,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_rf_configuration_rfon[] =
{
  0x00, 0x00, 0xff,
     4, _lcs,
  0xd4, PN532_CMD_RF_CONFIGURATION, 0x01, 0x03,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_rf_configuration_retries[] =
{
  0x00, 0x00, 0xff,
     6, _lcs,
  0xd4, PN532_CMD_RF_CONFIGURATION, 0x05, 0xff, 0x01, 0xff,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_in_list_passive_target[] =
{
  0x00, 0x00, 0xff,
     4, _lcs,
  0xd4, PN532_CMD_IN_LIST_PASSIVE_TARGET, 0x01, 0x00,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_get_data[] =
{
  0x00, 0x00, 0xff,
     3, _lcs,
  0xd4, PN532_CMD_TG_GET_DATA, 0x86,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_set_data[] =
{
  0x00, 0x00, 0xff,
    11, _lcs,
  0xd4, PN532_CMD_TG_SET_DATA,
  0x01, 0x02, 0x03, 0x04, 0x05,
  0x06, 0x07, 0x08, 0x09,
  _dcs, 0x00,
  0
};

const uint8_t PN532::packet_ack[] =
{
  0x00, 0x00, 0xff, 0x00, 0x0ff, 0x00
};

const uint8_t PN532::packet_nack[] =
{
  0x00, 0x00, 0xff, 0xff, 0x000, 0x00
};

const uint8_t PN532::packet_error[] =
{
  // ERROR is sent from PN532 to host to indicate an error was detected.
  0x00, 0x00, 0xff, 0x01, 0xff, 0x07f, 0x81, 0x00
};

