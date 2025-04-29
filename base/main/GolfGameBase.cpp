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
#include <string.h>
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
#include "GolfGameBase.h"
#include "NanoBeacon.h"
#include "NetworkServer.h"

GolfGameBase::GolfGameBase()
{
  memset(hits, 0, sizeof(hits));
}

GolfGameBase::~GolfGameBase()
{
}

void GolfGameBase::run()
{
  NetworkServer server;

  server.start();

  gpio_init();

  spi_init();
  display_clear();
  display_set_color(29, 0, 0);
  display_update(-1);

  NanoBeacon beacon;

  int current_player = -1;
  int hits = -1;
  bool is_connected = false;

  while (true)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Check if connection status changed so the color of the board
    // can be changed.
    if (is_connected)
    {
      if (server.is_connected() == false)
      {
        display_set_color(29, 0, 0);
        display_update(hits);
        is_connected = false;
      }
    }
      else
    {
      if (server.is_connected() == true)
      {
        if (current_player == -1)
        {
          display_set_color(0, 0, 29);
        }
          else
        {
          display_set_color(0, 29, 0);
        }

        display_update(hits);
        is_connected = true;
      }
    }

    int player = server.get_player();
    int player_index = player - 1;

    if (player != 0 && player_index != current_player)
    {
      ESP_LOGI(TAG, "New player %d (current=%d).", player_index, current_player);

      current_player = player_index;
      display_set_color(0, 29, 0);
      hits = 0;
      display_update(hits);

      play_song_begin();
    }

    if (current_player >= 0)
    {
      if (gpio_get_level(GPIO_HOLE) == 0)
      {
        hits++;
        this->hits[current_player] = hits;
        current_player = -1;
        display_set_color(0, 0, 29);
        display_update();
        play_song_finish();

      }
        else
      if (NanoBeacon::rotations[current_player].flags != 0)
      {
        NanoBeacon::rotations[current_player].clear_flags = true;

        hits++;

        ESP_LOGI(TAG, "current_player=%d hits=%d", current_player, hits);

        display_update(hits);
        play_song_hit();
      }
    }

    //gpio_set_level(GPIO_OUTPUT_IO_19, count % 2);
    //count++;
  }
}

void GolfGameBase::gpio_init()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Set GPIO_NUM_4 as input with pullup.
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_HOLE);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

  // Setup SPI chip select.
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_SPEAKER) |
    (1ULL << GPIO_SPI_CS)  |
    (1ULL << GPIO_SPI_SCK) |
    (1ULL << GPIO_SPI_DO);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

  gpio_config(&io_conf);

  // GPIO_NUM_0: Speaker.
  // GPIO_NUM_4: /CS
  // GPIO_NUM_5: DI
  // GPIO_NUM_6: SCK
  // GPIO_NUM_7: DO
  gpio_set_level(GPIO_SPEAKER, 0);
  gpio_set_level(GPIO_SPI_CS,  1);
  gpio_set_level(GPIO_SPI_DI,  0);
  gpio_set_level(GPIO_SPI_SCK, 0);
  gpio_set_level(GPIO_SPI_DO,  0);

  memset(&io_conf, 0, sizeof(io_conf));

  // Set GPIO_NUM_5 as input.
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_SPI_DI);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
}

void GolfGameBase::spi_init()
{
  spi_bus_config_t spi_bus_config = { };
  spi_bus_config.sclk_io_num     = GPIO_SPI_SCK;
  spi_bus_config.mosi_io_num     = GPIO_SPI_DO;
  spi_bus_config.miso_io_num     = GPIO_SPI_DI;
#if 0
  spi_bus_config.data0_io_num    = -1;
  spi_bus_config.data1_io_num    = -1;
  spi_bus_config.data2_io_num    = -1;
  spi_bus_config.data3_io_num    = -1;
  spi_bus_config.data4_io_num    = -1;
  spi_bus_config.data5_io_num    = -1;
  spi_bus_config.data6_io_num    = -1;
  spi_bus_config.data7_io_num    = -1;
#endif
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

void GolfGameBase::spi_send(uint8_t ch)
{
  //spi_send_sw(ch);
  //return;

  spi_transaction_t trans_desc = { };
  trans_desc.cmd = 0;
  trans_desc.length = 8;
  trans_desc.tx_buffer = &ch;
  //trans_desc.tx_data[0] = ch;
  //trans_desc.tx_data[1] = ch;
  //trans_desc.tx_data[2] = ch;
  //trans_desc.tx_data[3] = ch;

  spi_device_transmit(spi_handle, &trans_desc);
}

void GolfGameBase::spi_send_sw(uint8_t ch)
{
  int i;
  int data = ch & 0xff;

  for (i = 0; i < 8; i++)
  {
    if ((data & 0x80) != 0)
    {
      gpio_set_level(GPIO_SPI_DO,  1);
    }

    data = data << 1;

    gpio_set_level(GPIO_SPI_SCK, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_SPI_SCK, 0);

    gpio_set_level(GPIO_SPI_DO, 0);
  }
}

void GolfGameBase::display_clear()
{
  gpio_set_level(GPIO_SPI_CS, 0);

  // 0x7c, 0x2d: Clear.
  spi_send('|');
  spi_send(0x2d);

  gpio_set_level(GPIO_SPI_CS, 1);
}

void GolfGameBase::display_show(const char *text)
{
  gpio_set_level(GPIO_SPI_CS, 0);

  while (*text != 0)
  {
    spi_send(*text);
    text++;
  }

  gpio_set_level(GPIO_SPI_CS, 1);
}

void GolfGameBase::display_show(int value)
{
  gpio_set_level(GPIO_SPI_CS, 0);

  char digits[8];
  int ptr = 0;

  while (value > 0)
  {

    digits[ptr++] = value % 10;

    value = value / 10;
  }

  if (ptr == 0) { digits[ptr++] = 0; }

  while (ptr > 0)
  {
    spi_send(digits[--ptr] + '0');
  }

  gpio_set_level(GPIO_SPI_CS, 1);
}

void GolfGameBase::display_update(int value)
{
  display_clear();
  display_show("Player 1: ");
  display_show(hits[0]);
  display_show("\r");
  display_show("Player 2: ");
  display_show(hits[1]);
  display_show("\r");
  display_show("Player 3: ");
  display_show(hits[2]);
  display_show("\r");

  if (value >= 0)
  {
    display_show("Current Player:");
    display_show(value);
  }

  //display_show('+');
  //display_show(',');
}

void GolfGameBase::display_set_color(int r, int g, int b)
{
  if (r > 29) { r = 29; }
  if (g > 29) { g = 29; }
  if (b > 29) { b = 29; }

  gpio_set_level(GPIO_SPI_CS, 0);

  spi_send('|');
  spi_send(0x80 + r);
  spi_send('|');
  spi_send(0x9e + g);
  spi_send('|');
  spi_send(0xbc + b);

#if 0
  spi_send('|');
  spi_send('+');
  spi_send(r);
  spi_send(g);
  spi_send(b);
  spi_send(0);
#endif

  gpio_set_level(GPIO_SPI_CS, 1);
}

void GolfGameBase::set_tone(int frequency)
{
  if (frequency == 0)
  {
    // Turn off sound.
    gpio_set_level(GPIO_SPEAKER, 0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    return;
  }

  ledc_timer_config_t ledc_timer = { };
  ledc_timer.speed_mode      = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num       = LEDC_TIMER_0;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz         = frequency;
  ledc_timer.clk_cfg         = LEDC_AUTO_CLK;

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = { };
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel    = LEDC_CHANNEL_0;
  ledc_channel.timer_sel  = LEDC_TIMER_0;
  ledc_channel.intr_type  = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num   = GPIO_SPEAKER;
  ledc_channel.duty       = frequency / 2;
  ledc_channel.hpoint     = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  gpio_set_level(GPIO_SPEAKER, 1);
}

void GolfGameBase::play_song_begin()
{
  // A4 G5.
  set_tone(440);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(784);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(0);
}

void GolfGameBase::play_song_hit()
{
  // G5 A4.
  set_tone(784);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(440);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(0);
}

void GolfGameBase::play_song_finish()
{
  // G4 B4 D5 B4 G5
  set_tone(392);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(494);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(587);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(494);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(784);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(0);
}

const char *GolfGameBase::TAG = "BASE";

