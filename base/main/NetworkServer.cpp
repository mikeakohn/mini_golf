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
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "NetworkServer.h"

NetworkServer::NetworkServer() :
  control_pid       {  0 },
  control_socket_id { -1 },
  player            {  0 }
{
}

NetworkServer::~NetworkServer()
{
}

int NetworkServer::start()
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler,
    NULL,
    NULL));

  wifi_config_t wifi_config = { };

  strcpy((char *)wifi_config.ap.ssid, SSID);
  wifi_config.ap.ssid_len = sizeof(SSID) - 1;
  wifi_config.ap.channel = CHANNEL;
  strcpy((char *)wifi_config.ap.password, PASSWORD);
  wifi_config.ap.max_connection = 8;
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
  wifi_config.ap.authmode = WIFI_AUTH_WPA3_PSK;
  wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
#else
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
#endif
  wifi_config.ap.pmf_cfg.required = true;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(
    "wifi",
    "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    SSID, PASSWORD, CHANNEL);

  //pthread_create(&server_pid,  NULL, server_thread,  this);
  pthread_create(&control_pid, NULL, control_thread, this);

  return 0;
}

void NetworkServer::wifi_event_handler(
  void *arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void *event_data)
{
  if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event =
      (wifi_event_ap_staconnected_t *)event_data;

    ESP_LOGI(
      "wifi",
      "station " MACSTR " join, AID=%d",
      MAC2STR(event->mac),
      event->aid);
  }
    else
  if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event =
      (wifi_event_ap_stadisconnected_t *)event_data;

    ESP_LOGI(
      "wifi",
      "station " MACSTR " leave, AID=%d",
      MAC2STR(event->mac),
      event->aid);
  }
}

#if 0
void NetworkServer::server_response_status(int socket_id)
{
  char text[512];
  char header[256];
  char right[16];
  char left[16];

  right[0] = 0;
  left[0] = 0;

  sprintf(
    text,
    "{ "
    " \"right_fault\": %s,"
    " \"left_fault\": %s,"
    "}",
    right,
    left,

  int length = strlen(text);

  sprintf(
    header,
    "HTTP/1.1 200 OK\n"
    "Cache-Control: no-cache, must-revalidate\n"
    "Pragma: no-cache\n"
    "Content-Type: text/html\n"
    "Content-Length: %d\n\n",
    length);

  Network::net_send(socket_id, (const uint8_t *)header, strlen(header));
  Network::net_send(socket_id, (const uint8_t *)text, length);
}
#endif

#if 0
int NetworkServer::server_get_response(int socket_id)
{
  char buffer[32];
  int response = 0;

  while (true)
  {
    int length = net_recv(socket_id, (uint8_t *)buffer, sizeof(buffer));
    if (length <= 0) { return -1; }

    for (int n = 0; n < length; n++)
    {
      response = buffer[n];
    }

    if (response >= 1 && response <= 3) { break; }
  }

  return response;
}
#endif

#if 0
void NetworkServer::server_process(int socket_id)
{
  char buffer[1];

  while (true)
  {
    int length = net_recv(socket_id, (uint8_t *)buffer, sizeof(buffer));

    if (reponse < 0) { return; }
  }
}
#endif

#if 0
void NetworkServer::server_run()
{
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  int socket_id = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_id < 0)
  {
    printf("Can't open socket.\n");
    return;
  }

  memset((char*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(HTTP_PORT);

  if (bind(socket_id, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Server can't bind.\n");
    return;
  }

  if (listen(socket_id, 1) != 0)
  {
    printf("Listen failed.\n");
    return;
  }

  while (true)
  {
    socklen_t n = sizeof(client_addr);
    int client = accept(socket_id, (struct sockaddr *)&client_addr, &n);

    if (client == -1) { continue; }
    fcntl(client, F_SETFL, O_NONBLOCK);

    server_process(client);
    Network::net_close(client);
  }
}
#endif

void NetworkServer::control_process(int socket_id)
{
  int n;
  uint8_t buffer[1];
  struct timeval tv;
  fd_set readset;

  control_socket_id = socket_id;

  while (true)
  {
    //length = 0;

    //while (length < 5)
    {
      FD_ZERO(&readset);
      FD_SET(socket_id, &readset);

      tv.tv_sec = 2;
      tv.tv_usec = 0;

      n = select(socket_id + 1, &readset, NULL, NULL, &tv);

      if (n == -1)
      {
        if (errno == EINTR) { continue; }
        return;
      }

      // Timeout on select().
      if (n == 0) { continue; }

      //n = recv(socket_id, buffer + length, sizeof(buffer) - length, 0);
      n = recv(socket_id, buffer, sizeof(buffer), 0);

      ESP_LOGI("control_run", "Read %d bytes %02x.", n, buffer[0]);

      if (n == 0) { continue; }

      if (n < 0)
      {
        if (errno == ENOTCONN) { return; }
        continue;
      }

      if (buffer[0] >= 1 && buffer[0] <= 3)
      {
        set_player(buffer[0]);
      }

      //length += n;
    }
  }

  control_socket_id = -1;
}

void NetworkServer::control_run()
{
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  int socket_id = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_id < 0)
  {
    printf("Can't open socket.\n");
    return;
  }

  memset((char*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(CONTROL_PORT);

  if (bind(socket_id, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Server can't bind.\n");
    return;
  }

  if (listen(socket_id, 1) != 0)
  {
    printf("Listen failed.\n");
    return;
  }

  while (true)
  {
    socklen_t n = sizeof(client_addr);
    int client = accept(socket_id, (struct sockaddr *)&client_addr, &n);

    if (client == -1) { continue; }
    fcntl(client, F_SETFL, O_NONBLOCK);

    ESP_LOGI("control_run", "New connection socket_id=%d", client);
    control_process(client);
    ESP_LOGI("control_run", "Disconnect.");

    Network::net_close(client);
  }
}

#if 0
void *NetworkServer::server_thread(void *context)
{
  NetworkServer *network_server = (NetworkServer *)context;
  network_server->server_run();

  return NULL;
}
#endif

void *NetworkServer::control_thread(void *context)
{
  NetworkServer *network_server = (NetworkServer *)context;
  network_server->control_run();

  return NULL;
}

