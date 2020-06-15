
#include <esp_eth.h>
#include <tcpip_adapter.h>
#include <esp_event.h>
#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include "tcpip_adapter.h"

#define LOG_LOCAL_LEVEL  3
#include <esp_log.h>
#include <cstring>
#include "ethernet.h"
#include "artnet.h"
//#define ART_NET_PORT 6454

static const char *TAG = "gw-ethernet";
EventGroupHandle_t ethernet::eth_event_group = 0;
const int ethernet::GOTIP_BIT = BIT0;


void ethernet::eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    uint8_t mac_addr[6] = {0};
    
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
        case ETHERNET_EVENT_CONNECTED:
            esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
            ESP_LOGI(TAG, "Ethernet Link Up");
            ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
            break;
        case ETHERNET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Ethernet Link Down");
            xEventGroupClearBits(ethernet::eth_event_group, GOTIP_BIT);
            break;
        case ETHERNET_EVENT_START:
            ESP_LOGI(TAG, "Ethernet Started");
            break;
        case ETHERNET_EVENT_STOP:
            ESP_LOGI(TAG, "Ethernet Stopped");
            xEventGroupClearBits(ethernet::eth_event_group, GOTIP_BIT);
            break;
        default:
            break;
    }

}

/** Event handler for IP_EVENT_ETH_GOT_IP */
void ethernet::got_ip_event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const tcpip_adapter_ip_info_t *ip_info = &event->ip_info;
    xEventGroupSetBits(ethernet::eth_event_group, GOTIP_BIT);

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
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
        dest_addr.sin_port = htons(ART_NET_PORT); 
        
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
        ESP_LOGI(TAG, "Socket bound, port %d", ART_NET_PORT);

        while (1) {

            //ESP_LOGI(TAG, "Waiting for data");
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

                uint16_t opcode = artnet::parse_udp_buffer(rx_buffer, sizeof(rx_buffer), (struct sockaddr_in *)&source_addr);

                if(opcode == ART_POLL) {

                    artnet::artnet_reply_s *replyS = artnet::get_reply();

                    err = sendto(sock, (uint8_t *)replyS, sizeof(artnet::artnet_reply_s), 0, (struct sockaddr *)&source_addr, sizeof(source_addr));

                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                    }
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

    artnet::init_poll_reply();

    tcpip_adapter_init();
    
    ESP_ERROR_CHECK(tcpip_adapter_set_default_eth_handlers());
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    eth_event_group = xEventGroupCreate();

    esp_err_t ret = ESP_OK;


    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    mac_config.smi_mdc_gpio_num = 23;
    mac_config.smi_mdio_gpio_num = 18;
    mac_config.sw_reset_timeout_ms = 400;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);

    
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 0;
    phy_config.reset_gpio_num = 5;
    phy_config.reset_timeout_ms = 800;
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;

    vTaskDelay(500 / portTICK_PERIOD_MS);

    ret = esp_eth_driver_install(&config, &eth_handle);

    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "ERROR esp_eth_driver_install\n");
    } else {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        ret = esp_eth_start(eth_handle);
    }
    
    return ret;
}
