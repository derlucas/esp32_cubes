#ifndef GATEWAY_FIRMWARE_ESPNOWHANDLER_H
#define GATEWAY_FIRMWARE_ESPNOWHANDLER_H

#include "esp_now.h"

#define ESPNOW_CHANNEL   1
#define PREAMBLE            0xAABB
#define CUBE_ADDR_BCAST     0xff        // light broadcast address
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02
#define COMMAND_DEF_COLOR   0x03
#define COMMAND_COLOR_BCAST 0x04

class espnowhandler {

public:
    static void send_blackout(bool blackout);

    static void send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue);

    static void send_default_color_command(uint8_t uid);

    static void send_color_broadcast(uint16_t lightsCount, const uint8_t *fadeTimeRGBData);

    static esp_err_t init();

    static esp_err_t event_handler(void *ctx, system_event_t *event);

private:


    static void wifi_init(void);
    static void msg_send_cb(const uint8_t* mac, esp_now_send_status_t sendStatus);
    static esp_err_t esp_now_send_wrapper(const uint8_t *peer_addr, const uint8_t *data, size_t len);


    static uint32_t commandcounter;

    /* User defined field of ESPNOW data */
    typedef struct {
        uint16_t preamble;
        uint8_t uid;
        uint32_t commandcounter;
        uint8_t command;
        uint8_t crc;
        uint8_t payload[4 * 50];
    } lightcontrol_espnow_data_t;

    static lightcontrol_espnow_data_t espnow_data;

    static uint8_t broadcast_mac[ESP_NOW_ETH_ALEN];

    static EventGroupHandle_t espnow_event_group;
    static const int SEND_SUCCESS_BIT;

};


#endif //GATEWAY_FIRMWARE_ESPNOWHANDLER_H
