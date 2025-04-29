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

#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <pthread.h>

#include "esp_event.h"
#include "esp_wifi.h"

#include "Network.h"

class NetworkClient : public Network
{
public:
  NetworkClient();
  ~NetworkClient();

  int start();
  int start_network_thread();
  int start_wifi();
  int start_player(int value);

  bool is_connected() { return socket_id > 0; }

private:
  static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data);

  int net_connect();
  void control_run();

  static void *control_thread(void *context);

  pthread_t control_pid;
  int socket_id;
};

#endif

