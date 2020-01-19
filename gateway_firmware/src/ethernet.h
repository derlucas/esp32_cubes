#ifndef GATEWAY_FIRMWARE_ETHERNET_H
#define GATEWAY_FIRMWARE_ETHERNET_H

#include <esp_err.h>
#include <freertos/event_groups.h>


class ethernet {

public:

    static esp_err_t init_ethernet();
    static void udp_server_task(void *pvParameters);
    static esp_err_t eth_event_handler(void *ctx, system_event_t *event);


private:

    static void phy_device_power_enable_via_gpio(bool enable);
    static void eth_gpio_config_rmii();

    static EventGroupHandle_t eth_event_group;
    static const int GOTIP_BIT;

};


#endif //GATEWAY_FIRMWARE_ETHERNET_H
