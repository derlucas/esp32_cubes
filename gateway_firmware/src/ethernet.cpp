
#include <esp_eth.h>
#include <eth_phy/phy_lan8720.h>
#include <tcpip_adapter.h>
#include <esp_event.h>
#include <esp_event_loop.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>

#define LOG_LOCAL_LEVEL  3
#include <esp_log.h>
#include <cstring>
#include "ethernet.h"

static const char *TAG = "gw-ethernet";
EventGroupHandle_t ethernet::eth_event_group = 0;
const int ethernet::GOTIP_BIT = BIT0;

esp_err_t ethernet::eth_event_handler(void *ctx, system_event_t *event) {
    tcpip_adapter_ip_info_t ip;

    switch (event->event_id) {
        case SYSTEM_EVENT_ETH_CONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Up");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down");
            xEventGroupClearBits(ethernet::eth_event_group, GOTIP_BIT);
            break;
        case SYSTEM_EVENT_ETH_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
            ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ip));
            xEventGroupSetBits(ethernet::eth_event_group, GOTIP_BIT);
            ESP_LOGI(TAG, "Ethernet Got IP Addr");
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip.ip));
            ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip.netmask));
            ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            xEventGroupClearBits(ethernet::eth_event_group, GOTIP_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void ethernet::phy_device_power_enable_via_gpio(bool enable) {
    if (!enable) {
        phy_lan8720_default_ethernet_config.phy_power_enable(false);
    }

    gpio_pad_select_gpio(GPIO_NUM_5);
    gpio_set_direction(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_5, (int) enable);

    // Allow the power up/down to take effect, min 300us
    vTaskDelay(1);

    if (enable) {
        phy_lan8720_default_ethernet_config.phy_power_enable(true);
    }
}

void ethernet::eth_gpio_config_rmii() {
    // RMII data pins are fixed:
    // TXD0 = GPIO19
    // TXD1 = GPIO22
    // TX_EN = GPIO21
    // RXD0 = GPIO25
    // RXD1 = GPIO26
    // CLK == GPIO0
    phy_rmii_configure_data_interface_pins();
    // MDC is GPIO 23, MDIO is GPIO 18
    phy_rmii_smi_configure_pins(GPIO_NUM_23, GPIO_NUM_18);
}

void ethernet::udp_server_task(void *pvParameters) {
    char rx_buffer[1500];
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    while (1) {

        /* acquiring for ip, could blocked here */
        xEventGroupWaitBits(eth_event_group, GOTIP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

        struct sockaddr_in dest_addr{};
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(1234);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);


        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err < 0) {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", 1234);

        while (1) {

            ESP_LOGI(TAG, "Waiting for data");
            struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
            socklen_t socklen = sizeof(source_addr);
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                break;
            }
                // Data received
            else {
                // Get the sender's ip address as string
//                if (source_addr.sin6_family == PF_INET) {
//                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
//                } else if (source_addr.sin6_family == PF_INET6) {
//                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
//                }
//
//                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string...
//                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
//                ESP_LOGI(TAG, "%s", rx_buffer);



                int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                if (err < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    break;
                }
            }
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }

    vTaskDelete(nullptr);
}

esp_err_t ethernet::init_ethernet() {
    eth_event_group = xEventGroupCreate();

    eth_config_t config = phy_lan8720_default_ethernet_config;
    esp_err_t ret = ESP_OK;

    /* Initialize adapter */
    tcpip_adapter_init();
//    ret = esp_event_loop_init(ethernet::eth_event_handler, nullptr);
//    if(ret != ESP_OK) {
//        ESP_LOGE(TAG, "error esp event loop %d", ret);
//        return ret;
//    }

    config.phy_addr = PHY0;
    config.gpio_config = eth_gpio_config_rmii;
    config.tcpip_input = tcpip_adapter_eth_input;
    config.phy_power_enable = phy_device_power_enable_via_gpio;
    config.clock_mode = ETH_CLOCK_GPIO17_OUT;

    /* Initialize ethernet */
    ret = esp_eth_init(&config);
    if (ret != ESP_OK)
        return ret;

    ret = esp_eth_enable();
    vTaskDelay(100 / portTICK_PERIOD_MS);
    return ret;
}

