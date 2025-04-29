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

#ifndef GOLF_GAME_TEE_H
#define GOLF_GAME_TEE_H

#include <string.h>

#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "NetworkClient.h"

class GolfGameTee
{
public:
  GolfGameTee();
  ~GolfGameTee();

  void run();

private:
  void gpio_init();
  //void spi_init();
  uint8_t spi_send(uint8_t ch);
  void spi_send_packet(const uint8_t *packet, int length);
  int  spi_receive(uint8_t *data, int length);
  int  spi_receive_packet(uint8_t *packet, int length);
  bool wait_for_rfid_irq_with_timeout();
  void rfid_init();

  void rfid_send_packet(const uint8_t *packet)
  {
    int length = packet[3] + 5 + 2;
    spi_send_packet(packet, length);
  }

  void rfid_send_data_writing_byte();
  void rfid_send_data_reading_byte();
  int  rfid_receive_ack();
  void rfid_transmit_ack();
  //void rfid_get_firmware_version();

  void rfid_send_and_get_response(
    const uint8_t *packet,
    uint8_t *response,
    int length);

  int  rfid_validate_packet(const uint8_t *packet);

  //NetworkClient network_client;

  spi_device_handle_t spi_handle;

  static const char *TAG;
};

#endif

