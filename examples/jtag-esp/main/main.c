#include <errno.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/portmacro.h"
#include "lwip/api.h"
#include "lwip/netdb.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

static esp_err_t event_handler(void *ctx, system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      printf("got ip\n");
      printf("ip: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.ip));
      printf("netmask: " IPSTR "\n",
             IP2STR(&event->event_info.got_ip.ip_info.netmask));
      printf("gw: " IPSTR "\n", IP2STR(&event->event_info.got_ip.ip_info.gw));
      printf("\n");
      fflush(stdout);
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      esp_wifi_connect();
      break;
    default:
      break;
  }
  return ESP_OK;
}

#define SWCLK GPIO_NUM_4
#define SWDIO GPIO_NUM_13
#define NRST GPIO_NUM_16
#define BLINK GPIO_NUM_17

static void handleConnection(struct netconn *conn) {
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  printf("Received ARM programming request.\n");
  gpio_pad_select_gpio(SWDIO);
  gpio_pad_select_gpio(SWCLK);
  gpio_pad_select_gpio(NRST);
  gpio_set_direction(SWDIO, GPIO_MODE_OUTPUT);
  gpio_set_direction(SWCLK, GPIO_MODE_OUTPUT);
  gpio_set_direction(NRST, GPIO_MODE_OUTPUT);
  gpio_set_direction(BLINK, GPIO_MODE_OUTPUT);
  gpio_set_level(BLINK, 1);
  gpio_set_level(SWCLK, 0);
  gpio_set_level(SWDIO, 1);
  gpio_set_level(NRST, 1);
  while (1) {
    err = netconn_recv(conn, &inbuf);
    if (err != ERR_OK) {
      break;
    }
    netbuf_data(inbuf, (void **)&buf, &buflen);
    // printf("received %i\n", buflen);
    for (u16_t i = 0; i < buflen; i++) {
      switch (buf[i]) {
        case '0':
          gpio_set_level(SWDIO, 0);
          gpio_set_level(SWCLK, 0);
          // printf("CK = 0 IO = 0\n");
          break;
        case '4':
          gpio_set_level(SWDIO, 0);
          gpio_set_level(SWCLK, 1);
          // printf("CK = 1 IO = 0\n");
          break;
        case '2':
          gpio_set_level(SWDIO, 1);
          gpio_set_level(SWCLK, 0);
          // printf("CK = 0 IO = 1\n");
          break;
        case '6':
          gpio_set_level(SWDIO, 1);
          gpio_set_level(SWCLK, 1);
          // printf("CK = 1 IO = 1\n");
          break;
        case 'b':
          gpio_set_level(BLINK, 0);
          break;
        case 'B':
          gpio_set_level(BLINK, 1);
          break;
        case 'r':
        case 't':
          // printf("unreset\n");
          gpio_set_level(NRST, 1);
          break;
        case 's':
        case 'u':
          // printf("reset\n");
          gpio_set_level(NRST, 0);
          break;
        case 'R':
        case 'G':
          // printf("read = ");
          if (gpio_get_level(SWDIO)) {
            // printf("1\n");
            netconn_write(conn, "1", 1, NETCONN_NOCOPY);
          } else {
            // printf("0\n");
            netconn_write(conn, "0", 1, NETCONN_NOCOPY);
          }
          break;
        case 'D':
          // printf("drive on\n");
          gpio_set_direction(SWDIO, GPIO_MODE_OUTPUT);
          break;
        case 'd':
          // printf("drive off\n");
          gpio_set_direction(SWDIO, GPIO_MODE_INPUT);
          break;
        case 'Q':
          netbuf_delete(inbuf);
          // printf("quit\n");
          goto endLoop;
      }
    }
    netbuf_delete(inbuf);
  }
endLoop:
  gpio_set_level(NRST, 1);
  gpio_set_level(BLINK, 0);
  printf("ARM programming done.\n");
}

static void jtagProgrammer(void *pvParameters) {
  struct netconn *conn, *newconn;
  err_t err;
  conn = netconn_new(NETCONN_TCP);
  netconn_bind(conn, NULL, 7777);
  netconn_listen(conn);
  do {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      handleConnection(newconn);
      netconn_delete(newconn);
    }
  } while (1);
  netconn_close(conn);
  netconn_delete(conn);
}

void app_main(void) {
  nvs_flash_init();
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  // ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  // ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  // wifi_config_t sta_config = {.sta = {.ssid = "ssid",
  //                                     .password =
  //                                     "pwd",
  //                                     .bssid_set = false}};
  // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
  // ESP_ERROR_CHECK(esp_wifi_start());
  // ESP_ERROR_CHECK(esp_wifi_connect());
  xTaskCreate(&dupa, "jtagProgrammer", 4096, NULL, 5, NULL);
}
