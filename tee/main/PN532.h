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

#ifndef PN532_H
#define PN532_H

#include <stdint.h>

// Commands.

// Miscellaneous.
#define PN532_CMD_DIAGNOSE                  0x00
#define PN532_CMD_GET_FIRMWARE_VERSION      0x02
#define PN532_CMD_GET_GENERAL_STATUS        0x04
#define PN532_CMD_GET_READ_REGISTER         0x06
#define PN532_CMD_GET_WRITE_REGISTER        0x08
#define PN532_CMD_READ_GPIO                 0x0c
#define PN532_CMD_WRITE_GPIO                0x0e
#define PN532_CMD_SET_SERIAL_BAUD_RATE      0x10
#define PN532_CMD_SET_PARAMETERS            0x12
#define PN532_CMD_SAM_CONFIGURATION         0x14
#define PN532_CMD_POWER_DOWN                0x16

// RF Communication.
#define PN532_CMD_RF_CONFIGURATION          0x32
#define PN532_CMD_RF_REGULATION_TEST        0x58

// Initiator.
#define PN532_CMD_IN_JUMP_FOR_DEP           0x56
#define PN532_CMD_IN_JUMP_FOR_PSL           0x46
#define PN532_CMD_IN_LIST_PASSIVE_TARGET    0x4a
#define PN532_CMD_IN_ATR                    0x50
#define PN532_CMD_IN_PSL                    0x4e
#define PN532_CMD_IN_DATA_EXCHANGE          0x40
#define PN532_CMD_IN_COMMUNICATE_THRU       0x42
#define PN532_CMD_IN_DESELECT               0x44
#define PN532_CMD_IN_RELEASE                0x52
#define PN532_CMD_IN_SELECT                 0x54
#define PN532_CMD_IN_AUTO_POLL              0x60

// Target.
#define PN532_CMD_TG_INIT_AS_TARGET         0x8c
#define PN532_CMD_TG_SET_GENERAL_BYTES      0x92
#define PN532_CMD_TG_GET_DATA               0x86
#define PN532_CMD_TG_SET_DATA               0x8e
#define PN532_CMD_TG_SET_META_DATA          0x94
#define PN532_CMD_TG_GET_INITIATOR_COMMAND  0x88
#define PN532_CMD_TG_RESPONSE_TO_INITIATOR  0x90
#define PN532_CMD_TG_GET_TARGET_STATUS      0x8a

// Error codes (7.1 Page 67).
#define PN532_ERROR_TIMEOUT              0x01
#define PN532_ERROR_CRC                  0x02
#define PN532_ERROR_PARITY               0x03
#define PN532_ERROR_BIT_COUNT            0x04
#define PN532_ERROR_FRAME                0x05
#define PN532_ERROR_COLLISION            0x06
#define PN532_ERROR_BUFER_SIZE           0x07
#define PN532_ERROR_OVERFLOW             0x09
#define PN532_ERROR_INACTIVE             0x0a
#define PN532_ERROR_RF_PROTOCL           0x0b
#define PN532_ERROR_TEMPERATURE          0x0d
#define PN532_ERROR_INTERNAL_OVERFLOW    0x0e
#define PN532_ERROR_INVALID_PARAM        0x10
#define PN532_ERROR_DEP_INVALID_COMMAND  0x12
#define PN532_ERROR_DEP_DATA_FORMAT      0x13
#define PN532_ERROR_AUTH_ERROR           0x14
#define PN532_ERROR_CHECK_BYTE           0x23
#define PN532_ERROR_DEP_BAD_STATE        0x25
#define PN532_ERROR_ILLEGAL_OP           0x26
#define PN532_ERROR_CONTEXT              0x27
#define PN532_ERROR_RELEASED             0x29
#define PN532_ERROR_ID_MISMATCH          0x2a
#define PN532_ERROR_DISAPPEARED          0x2b
#define PN532_ERROR_NFCID_MISMATCH       0x2c
#define PN532_ERROR_OVERCURRENT          0x2d
#define PN532_ERROR_DEP_MISSING_NAD      0x2e

class PN532
{
public:
  static const uint8_t packet_sam_config[];

  static const uint8_t packet_get_firmware_version[];
  static const uint8_t packet_rf_configuration_rfon[];
  static const uint8_t packet_rf_configuration_retries[];
  static const uint8_t packet_in_list_passive_target[];
  static const uint8_t packet_get_data[];
  static const uint8_t packet_set_data[];
  static const uint8_t packet_ack[];
  static const uint8_t packet_nack[];
  static const uint8_t packet_error[];

private:
  PN532();
  ~PN532();

};

#endif

