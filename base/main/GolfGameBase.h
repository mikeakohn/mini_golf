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

#ifndef GOLF_GAME_BASE_H
#define GOLF_GAME_BASE_H

#include <string.h>

#include "driver/spi_master.h"
#include "driver/spi_common.h"

class GolfGameBase
{
public:
  GolfGameBase();
  ~GolfGameBase();

  void run();

private:
  void gpio_init();
  void spi_init();
  void spi_send(uint8_t ch);
  void spi_send_sw(uint8_t ch);
  void display_clear();
  void display_show(const char *text);
  void display_show(int value);
  void display_update(int value = -1);
  void display_set_color(int r, int g, int b);
  void set_tone(int frequency);
  void play_song_begin();
  void play_song_hit();
  void play_song_finish();

  //void run_game(int player);

  spi_device_handle_t spi_handle;

  int hits[3];

  static const char *TAG;
};

#endif

