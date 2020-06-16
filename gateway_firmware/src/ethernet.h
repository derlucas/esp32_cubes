#ifndef GATEWAY_FIRMWARE_ETHERNET_H
#define GATEWAY_FIRMWARE_ETHERNET_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_err.h>
#include <esp_event.h>


class ethernet {

public:

    static esp_err_t init_ethernet(uint8_t deviceCount);
    static void udp_server_task(void *pvParameters);

private:

    static void eth_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);
    static void got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);
                                    
    static void phy_device_power_enable_via_gpio(bool enable);
    static void eth_gpio_config_rmii();

    static EventGroupHandle_t eth_event_group;
    static const int GOTIP_BIT;

    static uint8_t deviceCount;

};


#endif //GATEWAY_FIRMWARE_ETHERNET_H
