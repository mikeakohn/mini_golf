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

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "driver/spi_common.h"
#include "soc/gpio_reg.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "rom/ets_sys.h"

#include "defines.h"
#include "GolfGameTee.h"
#include "PN532.h"

GolfGameTee::GolfGameTee()
{
}

GolfGameTee::~GolfGameTee()
{
}

void GolfGameTee::run()
{
  NetworkClient network_client;

  network_client.start();

  gpio_init();
  //spi_init();

  rfid_init();
  //rfid_get_firmware_version();

  uint8_t response[64];

  rfid_send_and_get_response(
    PN532::packet_get_firmware_version,
    response,
    sizeof(response));

  rfid_send_and_get_response(
    PN532::packet_rf_configuration_rfon,
    response,
    sizeof(response));

  rfid_send_and_get_response(
    PN532::packet_rf_configuration_retries,
    response,
    sizeof(response));

  while (true)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);

    // Waiting for a tag and reading the data is:
    // 1) Host sends to PN532 a InListPassiveTarget packet.
    // 2) Host waits for IRQ letting it know an ACK is available.
    // 3) If the ACK is positive, there is a data packet for response.
    //    If the ACK is negative (no data) try again.
    // 4) Wait for IRQ saying a packet is ready.
    // 5) Host reads tag data.

    rfid_send_packet(PN532::packet_in_list_passive_target);
    bool got_irq = wait_for_rfid_irq_with_timeout();

    if (got_irq == false)
    {
      //ESP_LOGE(TAG, "packet_send_and_get_response() no irq");
      //return;
      continue;
    }

    int status;

    status = rfid_receive_ack();
    if (status != 0) { continue; }

    ESP_LOGI(TAG, "main() ack status=%d", status);

    status = wait_for_rfid_irq_with_timeout();
    ESP_LOGI(TAG, "main() irq status=%d", status);

    uint8_t response[64];

    memset(response, 0, sizeof(response));
    int n = spi_receive_packet(response, sizeof(response));

    ESP_LOGI(TAG, "main() packet length %d", n);

    if (n > 0)
    {
      status = rfid_validate_packet(response);
      ESP_LOGI(TAG, "main() validate status=%d", status);
    }

    char debug[64];
    char temp[4];

    debug[0] = 0;

    for (int i = 0; i < n; i ++)
    {
      snprintf(temp, sizeof(temp), " %02x", response[i]);
      strcat(debug, temp);
    }

    ESP_LOGI(TAG, "main() %s", debug);

    static uint8_t player_1[] = { 0x3a, 0x00, 0xde, 0xf0 };
    static uint8_t player_2[] = { 0x31, 0x06, 0x41, 0x2d };
    static uint8_t player_3[] = { 0x2a, 0x00, 0xde, 0xf0 };

    if (memcmp(response + 13, player_1, 4) == 0)
    {
      network_client.start_player(1);
    }
      else
    if (memcmp(response + 13, player_2, 4) == 0)
    {
      network_client.start_player(2);
    }
      else
    if (memcmp(response + 13, player_3, 4) == 0)
    {
      network_client.start_player(3);
    }
  }
}

void GolfGameTee::gpio_init()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Setup SPI chip select.
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_SPI_CS) |
    //(1ULL << GPIO_SPI_DI) |
    (1ULL << GPIO_SPI_SCK)|
    (1ULL << GPIO_SPI_DO) |
    (1ULL << GPIO_RFID_RST);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

  gpio_config(&io_conf);

  // GPIO_NUM_4: /CS
  // GPIO_NUM_5: DI
  // GPIO_NUM_6: SCK
  // GPIO_NUM_7: DO
  // GPIO_NUM_8: IRQ
  // GPIO_NUM_9: /RST
  gpio_set_level(GPIO_SPI_CS,   1);
  gpio_set_level(GPIO_SPI_DI,   0);
  gpio_set_level(GPIO_SPI_SCK,  0);
  gpio_set_level(GPIO_SPI_DO,   0);
  gpio_set_level(GPIO_RFID_RST, 0);

  // Set GPIO_NUM_8 as input.
  memset(&io_conf, 0, sizeof(io_conf));

  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_SPI_DI) |
    (1ULL << GPIO_RFID_IRQ);
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
}

#if 0
void GolfGameTee::spi_init()
{
  spi_bus_config_t spi_bus_config = { };
  spi_bus_config.sclk_io_num     = GPIO_SPI_SCK;
  spi_bus_config.mosi_io_num     = GPIO_SPI_DO;
  spi_bus_config.miso_io_num     = GPIO_SPI_DI;
  spi_bus_config.quadwp_io_num   = -1;
  spi_bus_config.quadhd_io_num   = -1;
  spi_bus_config.max_transfer_sz = 1;

  spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t spi_dev_config = { };
  spi_dev_config.spics_io_num   = -1;
  spi_dev_config.command_bits   = 0;
  spi_dev_config.address_bits   = 0;
  spi_dev_config.mode           = 0;
  spi_dev_config.queue_size     = 7;
  //spi_dev_config.clock_source   = SPI_CLK_SRC_DEFAULT;
  spi_dev_config.clock_speed_hz = 100000;

  spi_bus_add_device(SPI2_HOST, &spi_dev_config, &spi_handle);
}
#endif

uint8_t GolfGameTee::spi_send(uint8_t ch)
{
  int i;
  int data_out = ch & 0xff;
  uint8_t data_in = 0;

  for (i = 0; i < 8; i++)
  {
    if ((data_out & 0x01) != 0)
    {
      gpio_set_level(GPIO_SPI_DO,  1);
    }

    data_out = data_out >> 1;

    gpio_set_level(GPIO_SPI_SCK, 1);
    ets_delay_us(10);

    data_in = data_in >> 1;
    data_in |= gpio_get_level(GPIO_SPI_DI) << 7;

    gpio_set_level(GPIO_SPI_SCK, 0);
    gpio_set_level(GPIO_SPI_DO, 0);
  }

  return data_in;
}

void GolfGameTee::spi_send_packet(const uint8_t *packet, int length)
{
  uint8_t data[length];
  memcpy(data, packet, length);

  int dcs = 0;
  const int index = 5 + data[3];

  // Compute LCS.
  data[4] = (0x100 - data[3]) & 0xff;

  for (int i = 5; i < index; i++)
  {
    //ESP_LOGI(TAG, "next %d> %02x %d", i, data[i], data[i]);
    dcs += data[i];
  }

  data[index] = (0x100 - (dcs & 0xff)) & 0xff;

  ESP_LOGI(TAG, "spi_send_packet() len=%d/%d dcs=%02x/%02x",
    data[3],
    data[4],
    data[index],
    dcs & 0xff);

  gpio_set_level(GPIO_SPI_CS, 0);

  rfid_send_data_writing_byte();

  for (int i = 0; i < length; i++)
  {
    //ESP_LOGI(TAG, "packet %d> %02x %d", i, data[i], data[i]);
    //dcs += data[i];
    spi_send(data[i]);
  }

  gpio_set_level(GPIO_SPI_CS, 1);
}

int GolfGameTee::spi_receive(uint8_t *data, int length)
{
  gpio_set_level(GPIO_SPI_CS, 0);

  rfid_send_data_reading_byte();

  for (int i = 0; i < length; i++)
  {
    data[i] = spi_send(0);
  }

  gpio_set_level(GPIO_SPI_CS, 1);

  return 0;
}

int GolfGameTee::spi_receive_packet(uint8_t *packet, int length)
{
  //esp_err_t err;

  int packet_len = -1;
  int retries = 0;
  int i = 0;

  gpio_set_level(GPIO_SPI_CS, 0);

  rfid_send_data_reading_byte();

  // FRAME: 0x00 0x00 0xff LEN LCS TFI PD0 PD1 ... PDn DCS 0x00
  //        LEN = includes a count of TFI PD0 to PDn

  if (length < 4) { return -1; }

  while (i < length)
  {
    packet[i] = spi_send(0);

    if (i == 0 && packet[i] != 0)
    {
      retries++;

      if (retries > 200)
      {
        ESP_LOGE(TAG, "spi_receive_packet() error - too many retries");
        return -1;
      }

      continue;
    }

    if (i == 3) { packet_len = packet[i]; };
    i++;

    if (i - 7 == packet_len) { break; }
  }

  gpio_set_level(GPIO_SPI_CS, 1);

  return i;
}

bool GolfGameTee::wait_for_rfid_irq_with_timeout()
{
  for (int i = 0; i < 10; i++)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (gpio_get_level(GPIO_RFID_IRQ) == 0) { return true; }
  }

  return false;
}

void GolfGameTee::rfid_init()
{
  // Initialize PN532 RFID chip.
  ESP_LOGI(TAG, "rfid_init()");

  // 1 second delay then raise /RESET so the chip wakes up.
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_RFID_RST, 1);

  // Hold CS low to take it out of low BAT mode into normal mode.
  gpio_set_level(GPIO_SPI_CS, 0);
  vTaskDelay(200 / portTICK_PERIOD_MS);

  ESP_LOGI(TAG, "rfid_init() start loop");

  while (true)
  {
    rfid_send_packet(PN532::packet_sam_config);
    if (wait_for_rfid_irq_with_timeout()) { break; }

    ESP_LOGW(TAG, "packet_sam_config() timeout");
  }

  ESP_LOGI(TAG, "packet_sam_config() sent");

  int status;
  status = rfid_receive_ack();
  ESP_LOGI(TAG, "packet_sam_config() ack status=%d", status);

  status = wait_for_rfid_irq_with_timeout();
  ESP_LOGI(TAG, "packet_sam_config() irq status=%d", status);

  uint8_t packet[64];
  int length = spi_receive_packet(packet, sizeof(packet));

  ESP_LOGI(TAG, "packet_sam_config() packet length %d", length);

  if (length > 0)
  {
    status = rfid_validate_packet(packet);
    ESP_LOGI(TAG, "packet_sam_config() validate status=%d", status);
  }
}

void GolfGameTee::rfid_send_data_writing_byte()
{
  // 6.2.5 (page 45) in the documentation explains this extra byte for SPI.

  uint8_t data = 0x01;
  spi_send(data);
}

void GolfGameTee::rfid_send_data_reading_byte()
{
  // 6.2.5 (page 45) in the documentation explains this extra byte for SPI.

  uint8_t data = 0x03;
  spi_send(data);
}

int GolfGameTee::rfid_receive_ack()
{
  uint8_t good_ack[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
  uint8_t packet[6];

  spi_receive(packet, 6);

  bool is_good = memcmp(packet, good_ack, 6) == 0;

#if 0
  if (is_good == false)
  {
    ESP_LOGE(TAG, "rfid_receive_ack() bad %02x %02x %02x %02x %02x %02x",
      packet[0],
      packet[1],
      packet[2],
      packet[3],
      packet[4],
      packet[5]);
  }
#endif

  return is_good ? 0 : -1;
}

void GolfGameTee::rfid_transmit_ack()
{
}

#if 0
void GolfGameTee::rfid_get_firmware_version()
{
  bool got_irq;
  int status;

  rfid_send_packet(PN532::packet_get_firmware_version);
  got_irq = wait_for_rfid_irq_with_timeout();

  if (got_irq == false)
  {
    ESP_LOGE(TAG, "packet_get_firmware_version() no irq");
    return;
  }

  status = rfid_receive_ack();
  ESP_LOGI(TAG, "packet_get_firmware_version() irq status=%d", status);

  status = wait_for_rfid_irq_with_timeout();
  ESP_LOGI(TAG, "packet_get_firmware_version() irq status=%d", status);

  uint8_t packet[64];
  int length = spi_receive_packet(packet, sizeof(packet));

  ESP_LOGI(TAG, "packet_get_firmware_version() packet length %d", length);

  if (length > 0)
  {
    status = rfid_validate_packet(packet);
    ESP_LOGI(TAG, "packet_get_firmware_version() validate status=%d", status);
  }
}
#endif

void GolfGameTee::rfid_send_and_get_response(
  const uint8_t *packet,
  uint8_t *response,
  int length)
{
  bool got_irq;
  int status;

  // Typical handshake is:
  // 1) Host sends to PN532 a command.
  // 2) Host waits for IRQ letting it know an ACK is available.
  // 3) If the ACK is positive, there is a data packet for response.
  // 4) Wait for IRQ saying a packet is ready.
  // 5) Host reads response.

  rfid_send_packet(packet);
  got_irq = wait_for_rfid_irq_with_timeout();

  if (got_irq == false)
  {
    ESP_LOGE(TAG, "rfid_send_and_get_response() no irq");
    return;
  }

  status = rfid_receive_ack();
  ESP_LOGI(TAG, "rfid_send_and_get_response() irq status=%d", status);

  status = wait_for_rfid_irq_with_timeout();
  ESP_LOGI(TAG, "rfid_send_and_get_response() irq status=%d", status);

  int n = spi_receive_packet(response, length);

  ESP_LOGI(TAG, "rfid_send_and_get_response() packet length %d", n);

  if (n > 0)
  {
    status = rfid_validate_packet(response);
    ESP_LOGI(TAG, "rfid_send_and_get_response() validate status=%d", status);
  }
}

int GolfGameTee::rfid_validate_packet(const uint8_t *packet)
{
  int len = packet[3];
  int lcs = packet[4];

  if (len != ((0x100 - lcs) & 0xff))
  {
    ESP_LOGE(TAG, "rfid_validate_packet() invalid len=%d lcs=%d", len, lcs);
    return -1;
  }

  if (packet[0] != 0x00 || packet[1] != 0x00 || packet[2] != 0xff)
  {
    ESP_LOGE(TAG, "rfid_validate_packet() invalid preamble=%02x %02x %02x",
      packet[0],
      packet[1],
      packet[2]);

    return -1;
  }

  int dcs = packet[len + 5];
  int postamble = packet[len + 6];

  ESP_LOGI(TAG, "rfid_validate_packet() packet_type=%d\n", packet[5]);
  ESP_LOGI(TAG, "rfid_validate_packet() dcs=%02x postamble=%d\n",
    dcs,
    postamble);

  int value = 0;
  for (int i = 5; i < len + 5; i++)
  {
    value += packet[i];
  }

  value = (0x100 - value) & 0xff;

  if (value != dcs)
  {
    ESP_LOGE(TAG, "rfid_validate_packet() invalid dcs=%02x vs %02x",
      dcs,
      value);
  }

  return 0;
}

const char *GolfGameTee::TAG = "TEE";

