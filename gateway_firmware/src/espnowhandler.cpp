#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <assert.h>
#include "esp_log.h"
#include <esp_now.h>

#include "espnowhandler.h"

#define CONFIG_ESPNOW_CHANNEL   0
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
#ifdef UART_DEBUG
    Serial.printf("send blackout %d\n", espnow_data.payload[0]);
#endif
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
    espnow_data->command = COMMAND_COLOR;
    espnow_data->commandcounter = ++commandcounter;
    espnow_data->payload[0] = fadetime;
    espnow_data->payload[1] = red;
    espnow_data->payload[2] = green;
    espnow_data->payload[3] = blue;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
#ifdef UART_DEBUG
    Serial.printf("send color %d,%d,%d,%d\n", espnow_data.payload[0],espnow_data.payload[1],espnow_data.payload[2],espnow_data.payload[3]);
#endif
}

void espnowhandler::send_default_color_command(uint8_t uid) {
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    espnow_data->uid = uid;
    espnow_data->command = COMMAND_DEF_COLOR;
    espnow_data->commandcounter = ++commandcounter;
    espnow_data->payload[0] = 1;
    esp_now_send(broadcast_mac, (const uint8_t *) espnow_data, sizeof(lightcontrol_espnow_data_t));
}


esp_err_t espnowhandler::init() {
    espnow_data = static_cast<lightcontrol_espnow_data_t *>(malloc(sizeof(lightcontrol_espnow_data_t)));
    memset(espnow_data, 0, sizeof(lightcontrol_espnow_data_t));
    if (espnow_data == nullptr) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        esp_now_deinit();
        return ESP_FAIL;
    }

    espnow_data->preamble = PREAMBLE;
    espnow_data->command = COMMAND_COLOR;

    ESP_ERROR_CHECK( esp_now_init() );

    /* Add broadcast peer information to peer list. */
    auto *peeer = static_cast<esp_now_peer_info_t *>(malloc(sizeof(esp_now_peer_info_t)));

    if (peeer == nullptr) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peeer, 0, sizeof(esp_now_peer_info_t));
    peeer->channel = CONFIG_ESPNOW_CHANNEL;
    peeer->ifidx = ESP_IF_WIFI_STA;         // configure to station mode
    peeer->encrypt = false;
    memcpy(peeer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peeer) );
    free(peeer);

    return ESP_OK;
}
