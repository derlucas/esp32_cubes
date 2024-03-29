#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <assert.h>
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"


#define LOG_LOCAL_LEVEL  1
#include "esp_log.h"
#include "espnowhandler.h"

static const char *TAG = "gateway-espnow";
uint8_t espnowhandler::broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint32_t espnowhandler::commandcounter = 0;
espnowhandler::lightcontrol_espnow_data_t espnowhandler::espnow_data;
EventGroupHandle_t espnowhandler::espnow_event_group = 0;
const int espnowhandler::SEND_SUCCESS_BIT = BIT0;

void espnowhandler::msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus) {
    xEventGroupSetBits(espnowhandler::espnow_event_group, SEND_SUCCESS_BIT);
    switch (sendStatus) {
        case ESP_NOW_SEND_SUCCESS:
            ESP_LOGD(TAG, "esp send done status = success");
            break;
        case ESP_NOW_SEND_FAIL:
            ESP_LOGD(TAG, "esp send done status = fail");
            break;
        default:
        break;
    }
}


esp_err_t espnowhandler::esp_now_send_wrapper(const uint8_t *peer_addr, const uint8_t *data, size_t len) {
    xEventGroupWaitBits(espnowhandler::espnow_event_group, SEND_SUCCESS_BIT, pdTRUE, pdTRUE, 120 / portTICK_PERIOD_MS);
    return esp_now_send(peer_addr, data, len);
}


/**
 * send blackout command via ESP-NOW
 * the command will be send three times via wireless for more reliability
 *
 * @param blackout true for blackout, false for restore
 */
void espnowhandler::send_blackout(bool blackout) {
    memset(&espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data.uid = CUBE_ADDR_BCAST;
    espnow_data.preamble = PREAMBLE;
    espnow_data.command = COMMAND_BLACKOUT;
    espnow_data.commandcounter = ++commandcounter;
    espnow_data.payload[0] = (uint8_t)(blackout ? 0x01 : 0x00);

    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    //vTaskDelay(20 / portTICK_PERIOD_MS);
    
    espnow_data.commandcounter = ++commandcounter;
    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    //vTaskDelay(20 / portTICK_PERIOD_MS);
    
    espnow_data.commandcounter = ++commandcounter;
    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send blackout %d\n", espnow_data.payload[0]);
    
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
    memset(&espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data.uid = uid;
    espnow_data.preamble = PREAMBLE;
    espnow_data.command = COMMAND_COLOR;
    espnow_data.commandcounter = ++commandcounter;
    espnow_data.payload[0] = fadetime;
    espnow_data.payload[1] = red;
    espnow_data.payload[2] = green;
    espnow_data.payload[3] = blue;
    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send color %d,%d,%d,%d\n", espnow_data.payload[0], espnow_data.payload[1],
                                                espnow_data.payload[2], espnow_data.payload[3]);
}

void espnowhandler::send_default_color_command(uint8_t uid) {
    memset(&espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data.uid = uid;
    espnow_data.preamble = PREAMBLE;
    espnow_data.command = COMMAND_DEF_COLOR;
    espnow_data.commandcounter = ++commandcounter;
    espnow_data.payload[0] = 1;
    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send default color to uid %d\n", espnow_data.uid);
}

/**
 * send color values via broadcast as big array
 * 
 * @param lightsCount       Number of lights to send data to. Maximum is 50
 * @param fadeTimeRGBData   An Array with four bytes for each light: [fadeTime, red, green, blue]
 */
void espnowhandler::send_color_broadcast(uint16_t lightsCount, const uint8_t *fadeTimeRGBData) {
    if(lightsCount > 50) return;

    memset(&espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data.uid = CUBE_ADDR_BCAST;
    espnow_data.preamble = PREAMBLE;
    espnow_data.command = COMMAND_COLOR_BCAST;
    espnow_data.commandcounter = ++commandcounter;

    memcpy(espnow_data.payload, fadeTimeRGBData, lightsCount * 4);

    esp_now_send_wrapper(broadcast_mac, (const uint8_t *) &espnow_data, sizeof(lightcontrol_espnow_data_t));
    ESP_LOGD(TAG, "send bcast color to %d lights\n", lightsCount);
}


esp_err_t espnowhandler::event_handler(void *ctx, system_event_t *event) {
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
    
//    ESP_ERROR_CHECK( esp_event_loop_init(example_event_handler, NULL) );
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
    peeer->ifidx = WIFI_IF_STA;         // configure to station mode
    peeer->encrypt = false;
    memcpy(peeer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peeer) );
    free(peeer);

    espnow_event_group = xEventGroupCreate();
    xEventGroupSetBits(espnow_event_group, SEND_SUCCESS_BIT);
    esp_now_register_send_cb(msg_send_cb);

    return ESP_OK;
}
