#ifndef GATEWAY_FIRMWARE_ESPNOWHANDLER_H
#define GATEWAY_FIRMWARE_ESPNOWHANDLER_H


class espnowhandler {

public:
    void send_blackout(bool blackout);

    void send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue);

    void send_default_color_command(uint8_t uid);

    esp_err_t init();

private:

    uint32_t commandcounter = 0;

    /* User defined field of ESPNOW data */
    typedef struct {
        uint16_t preamble;
        uint8_t uid;
        uint32_t commandcounter;
        uint8_t command;
        uint8_t crc;
        uint8_t payload[4];
    } lightcontrol_espnow_data_t;

    lightcontrol_espnow_data_t *espnow_data;

//    void esp_now_send_command(lightcontrol_espnow_data_t *pfoo);

    static void wifi_init(void);


};


#endif //GATEWAY_FIRMWARE_ESPNOWHANDLER_H
