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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "NetworkClient.h"

NetworkClient::NetworkClient() : socket_id { -1 }
{
}

NetworkClient::~NetworkClient()
{
}

int NetworkClient::start()
{
  start_wifi();
  start_network_thread();

  return 0;
}

int NetworkClient::start_wifi()
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

//esp_err_t err;
//err = esp_wifi_init(&cfg);
//  ESP_LOGI("WIFI", "err=%d\n", err);
//ESP_ERROR_CHECK(err);

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler,
    NULL,
    NULL));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    IP_EVENT,
    IP_EVENT_STA_GOT_IP,
    &wifi_event_handler,
    NULL,
    NULL));

  wifi_config_t wifi_config = { };

  strcpy((char *)wifi_config.sta.ssid, SSID);
  strcpy((char *)wifi_config.sta.password, PASSWORD);

  wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(
    "wifi",
    "wifi_init finished. SSID:%s password:%s channel:%d",
    SSID, PASSWORD, CHANNEL);

  return 0;
}

int NetworkClient::start_network_thread()
{
  pthread_create(&control_pid, NULL, control_thread, this);
  return 0;
}

int NetworkClient::start_player(int value)
{
  uint8_t buffer[1];

  buffer[0] = value;

  ESP_LOGI("wifi", "start_player(%d)", value);

  Network::net_send(socket_id, buffer, sizeof(buffer));

  return 0;
}

void NetworkClient::wifi_event_handler(
  void *arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void *event_data)
{
  if (event_id == WIFI_EVENT_STA_START)
  {
    ESP_LOGI(
      "wifi",
      "esp_wifi_connect();");

    esp_wifi_connect();
  }
    else
  if (event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    //sleep(2);
    esp_wifi_connect();

    ESP_LOGI("wifi", "Disconnected.. retry to connect to the AP");
  }
    else
  if (event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    ESP_LOGI(
      "wifi",
      "got ip:" IPSTR,
      IP2STR(&event->ip_info.ip));
  }
}

int NetworkClient::net_connect()
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0)
  {
    ESP_LOGI(
      "net_connect",
      "Can't create socket.\n");
    return -2;
  }

  struct sockaddr_in addr;
  memset((char*)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, "192.168.4.1", &addr.sin_addr);
  addr.sin_port = htons(CONTROL_PORT);

  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
  {
    ESP_LOGI(
      "net_connect",
      "Could not connect");
    close(sockfd);
    return -3;
  }

  fcntl(sockfd, F_SETFL, O_NONBLOCK);

  return sockfd;
}

void NetworkClient::control_run()
{
  while (true)
  {
    ESP_LOGI(
      "control_run",
      "Attempting to connect\n");

    socket_id = net_connect();

    ESP_LOGI(
      "control_run",
      "  .. connect socket_id=%d\n",
      socket_id);

    if (socket_id < 0)
    {
      socket_id = -1;
      sleep(4);
      continue;
    }

    ESP_LOGI("control_run", "Connected.\n");

    while (true)
    {
      uint8_t buffer[1];

      buffer[1] = 'X';

      int length = net_recv(socket_id, buffer, 1);

      if (length == 0 || length == -5) { continue; }
      if (length < 0) { break; }

#if 0
      switch (buffer[0])
      {
        case 'S':
          reset();
          break;
      }
#endif
    }

    ESP_LOGI("control_run", "Disconnected.\n");

    Network::net_close(socket_id);

    socket_id = -1;
  }
}

void *NetworkClient::control_thread(void *context)
{
  NetworkClient *network_end = (NetworkClient *)context;
  network_end->control_run();

  return NULL;
}

