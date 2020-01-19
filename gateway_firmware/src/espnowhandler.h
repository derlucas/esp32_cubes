#ifndef GATEWAY_FIRMWARE_ESPNOWHANDLER_H
#define GATEWAY_FIRMWARE_ESPNOWHANDLER_H


class espnowhandler {

public:
    static void send_blackout(bool blackout);

    static void send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue);

    static void send_default_color_command(uint8_t uid);

    static esp_err_t init();

    static esp_err_t event_handler(void *ctx, system_event_t *event);

private:


    static void wifi_init(void);


};


#endif //GATEWAY_FIRMWARE_ESPNOWHANDLER_H
