#ifndef GATEWAY_FIRMWARE_ESPNOWHANDLER_H
#define GATEWAY_FIRMWARE_ESPNOWHANDLER_H


class espnowhandler {

public:
    void send_blackout(bool blackout);

    void send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue);

    void send_default_color_command(uint8_t uid);

    void init();

private:
    uint8_t peer_addr[6];
    uint32_t commandcounter = 0;

    struct ESP_NOW_FOO {
        uint16_t preamble;
        uint8_t uid;
        uint32_t commandcounter;
        uint8_t command;
        uint8_t crc;
        uint8_t payload[4];
    };

    ESP_NOW_FOO espnow_data;

    void esp_now_send_command(ESP_NOW_FOO *pfoo);
};


#endif //GATEWAY_FIRMWARE_ESPNOWHANDLER_H
