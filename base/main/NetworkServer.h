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

#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H

#include <pthread.h>

#include "esp_event.h"
#include "esp_wifi.h"

#include "Network.h"

class NetworkServer : public Network
{
public:
  NetworkServer();
  ~NetworkServer();

  int start();
  int send_start_race();

  bool is_connected() { return control_socket_id != -1; }

  void set_player(int value)
  {
    pthread_mutex_lock(&lock);
    player = value;
    pthread_mutex_unlock(&lock);
  }

  int get_player()
  {
    int value;

    pthread_mutex_lock(&lock);
    value = player;
    player = 0; 
    pthread_mutex_unlock(&lock);

    return value;
  }

private:
  static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data);

  //int server_get_response(int socket_id);
  //void server_process(int socket_id);
  //void server_run();

  void control_process(int socket_id);
  void control_run();

  //static void *server_thread(void *context);
  static void *control_thread(void *context);

  //pthread_t server_pid;
  pthread_t control_pid;
  pthread_mutex_t lock;

  int control_socket_id;
  int player;
};

#endif

