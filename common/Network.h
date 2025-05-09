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

#ifndef NETWORK_H
#define NETWORK_H

#include <pthread.h>

#include "esp_event.h"
#include "esp_wifi.h"

#define SSID "minigolf"
#define PASSWORD "minigolf"
#define CHANNEL 6
//#define HTTP_PORT 80
#define CONTROL_PORT 8000

class Network
{
public:
  Network();
  ~Network();

protected:
  static int net_send(int socket_id, const uint8_t *buffer, int length);
  static int net_recv(int socket_id, uint8_t *buffer, int length);
  static void net_close(int socket_id);
};

#endif

