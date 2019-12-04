#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <assert.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "rom/crc.h"

#define LOG_LOCAL_LEVEL  1
#include "esp_log.h"
#include "espnowhandler.h"

#define ESPNOW_CHANNEL   1
#define PREAMBLE            0xAABB
#define CUBE_ADDR_BCAST     0xff        // light broadcast address
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02
#define COMMAND_DEF_COLOR   0x03

static const char *TAG = "gateway-espnow";
static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

/**
 * send blackout command via ESP-NOW
 * the command will be send three times via wireless for more reliability
 *
 * @param blackout true for blackout, false for restore
 */
void espnowhandler::send_blackout(bool blackout) {
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data->uid = CUBE_ADDR_BCAST;
    espnow_data->preamble = PREAMBLE;
    espnow_data->command = COMMAND_BLACKOUT;
    espnow_data->commandcounter = ++commandcounter;
    espnow_data->payload[0] = (uint8_t)(blackout ? 0x01 : 0x00);

    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
    vTaskDelay(20 / portTICK_PERIOD_MS);
    espnow_data->commandcounter = ++commandcounter;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
    vTaskDelay(20 / portTICK_PERIOD_MS);
    espnow_data->commandcounter = ++commandcounter;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send blackout %d\n", espnow_data->payload[0]);
}

/**
 * send set color command via ESP-NOW
 *
 * @param uid       Address of the light from 0 to 254, use 255 for broadcast
 * @param fadetime  time for fading, 0 = no fade, 255 = 2,55s fading (unit is centiseconds)
 * @param red       red color from 0 to 255
 * @param green     green color from 0 to 255
 * @param blue      blue color from 0 to 255
 */
void espnowhandler::send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue) {
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data->uid = uid;
    espnow_data->preamble = PREAMBLE;
    espnow_data->command = COMMAND_COLOR;
    espnow_data->commandcounter = ++commandcounter;
    espnow_data->payload[0] = fadetime;
    espnow_data->payload[1] = red;
    espnow_data->payload[2] = green;
    espnow_data->payload[3] = blue;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send color %d,%d,%d,%d\n", espnow_data->payload[0],espnow_data->payload[1],espnow_data->payload[2],espnow_data->payload[3]);
}

void espnowhandler::send_default_color_command(uint8_t uid) {
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data->uid = uid;
    espnow_data->preamble = PREAMBLE;
    espnow_data->command = COMMAND_DEF_COLOR;
    espnow_data->commandcounter = ++commandcounter;
    espnow_data->payload[0] = 1;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send default color to uid %d\n", espnow_data->uid);
}

static esp_err_t example_event_handler(void *ctx, system_event_t *event) {
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "WiFi started");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void espnowhandler::wifi_init() {
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(example_event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());

    /* In order to simplify example, channel is set after WiFi started.
     * This is not necessary in real application if the two devices have
     * been already on the same channel.
     */
    ESP_ERROR_CHECK( esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE) );
}

esp_err_t espnowhandler::init() {

    ESP_LOGD(TAG, "espnow init...");
    wifi_init();

    espnow_data = static_cast<lightcontrol_espnow_data_t *>(malloc(sizeof(lightcontrol_espnow_data_t)));
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    if (espnow_data == nullptr) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        esp_now_deinit();
        return ESP_FAIL;
    }
    ESP_LOGD(TAG, "malloc ok");

    espnow_data->preamble = PREAMBLE;
    espnow_data->command = COMMAND_COLOR;

    ESP_LOGD(TAG, "data set");
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_LOGD(TAG, "esp now init ok");

    /* Add broadcast peer information to peer list. */
    auto *peeer = static_cast<esp_now_peer_info_t *>(malloc(sizeof(esp_now_peer_info_t)));

    if (peeer == nullptr) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peeer, 0, sizeof(esp_now_peer_info_t));
    peeer->channel = ESPNOW_CHANNEL;
    peeer->ifidx = ESP_IF_WIFI_STA;         // configure to station mode
    peeer->encrypt = false;
    memcpy(peeer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peeer) );
    free(peeer);

    return ESP_OK;
}
