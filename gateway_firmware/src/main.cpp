#include <cstdio>
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "driver/uart.h"
#include "nvs_flash.h"

#include "esp_eth.h"

#define LOG_LOCAL_LEVEL  3
#define DEVICE_COUNT_DEFAULT 50

#include <esp_log.h>
#include "ethernet.h"
#include "espnowhandler.h"
#include "artnet.h"


#define PATTERN_LENGTH  (1)         //length of the pattern ("X")
#define BUF_SIZE        (1024)

static const char *TAG = "gateway";
static QueueHandle_t uart0_queue;


extern "C" {
void app_main(void);
}

void esp_handle_uart_pattern(uint8_t *dtmp) {
    size_t buffered_size;

    bzero(dtmp, BUF_SIZE);

    uart_get_buffered_data_len(UART_NUM_0, &buffered_size);
    int pos = uart_pattern_pop_pos(UART_NUM_0);

    ESP_LOGD(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);

    if (pos != -1) {
        uart_read_bytes(UART_NUM_0, dtmp, pos, 100 / portTICK_PERIOD_MS);
        uint8_t pat[PATTERN_LENGTH + 1];
        memset(pat, 0, sizeof(pat));
        uart_read_bytes(UART_NUM_0, pat, 1, 100 / portTICK_PERIOD_MS);
        ESP_LOGD(TAG, "read data: %s", dtmp);
        ESP_LOGD(TAG, "read pat : %s", pat);

        if (buffered_size >= 4 && dtmp[0] == 'A') {
            espnowhandler::send_blackout(dtmp[2] == '1');
            ESP_LOGD(TAG, "sending blackout %s", (dtmp[2] == '1') ? "true" : "false");
        } else if (buffered_size >= 12 && dtmp[0] == 'B') {
            ESP_LOGD(TAG, "sending color");
            int uid, fadetime, red, green, blue;
            sscanf(reinterpret_cast<const char *>(dtmp), "B,%d,%d,%d,%d,%dX", &uid, &fadetime, &red, &green, &blue);
            ESP_LOGD(TAG, "uid=%d  red=%d  green=%d  blue=%d  time=%d", uid, red, green, blue, fadetime);
            espnowhandler::send_color(uid, fadetime, red, green, blue);
        } else if (buffered_size >= 4 && dtmp[0] == 'C') {
            ESP_LOGD(TAG, "sending store default color");
            int uid;
            sscanf(reinterpret_cast<const char *>(dtmp), "C,%dX", &uid);
            ESP_LOGD(TAG, "uid=%d ", uid);
            espnowhandler::send_default_color_command(uid);
        }

    } else {
        ESP_LOGW(TAG, "Pattern Queue Size too small");
        uart_flush_input(UART_NUM_0);
    }


}

void uart_task(void *arg) {
    uart_event_t event;
    auto *dtmp = (uint8_t *) malloc(BUF_SIZE);

    for (;;) {
        //Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *) &event, (portTickType) portMAX_DELAY)) {

            ESP_LOGD(TAG, "uart[%d] event:", UART_NUM_0);
            switch (event.type) {
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "hw fifo overflow");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
                    break;
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "ring buffer full");
                    uart_flush_input(UART_NUM_0);
                    xQueueReset(uart0_queue);
                    break;
                case UART_PATTERN_DET: {
                    esp_handle_uart_pattern(dtmp);
                }
                    break;
                default:
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = nullptr;
    vTaskDelete(nullptr);
}

void app_main() {
    nvs_flash_init();

    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .use_ref_tick = false
    };
    uart_param_config(UART_NUM_0, &uart_config);

    uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Install UART driver, and get the queue.
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);

    //Set uart pattern detect function to detect newlines
    uart_enable_pattern_det_intr(UART_NUM_0, 'X', 1, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART_NUM_0, 10);

    ESP_LOGI(TAG, "Initializing Gateway...\n");

    espnowhandler::init();    
    artnet::set_listen_universe(5);

    xTaskCreate(uart_task, "uart_task", 2048, nullptr, 10, nullptr);

    int ret = ethernet::init_ethernet(DEVICE_COUNT_DEFAULT);
    if (ret == ESP_OK) {
        xTaskCreate(ethernet::udp_server_task, "udp_server_task", 4096, nullptr, (tskIDLE_PRIORITY + 2), nullptr);
    }
}

