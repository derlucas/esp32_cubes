#include <Arduino.h>
#include <esp_now.h>

#include "espnowhandler.h"

#define PREAMBLE            0xAABB
#define CUBE_ADDR_BCAST     0xff        // light broadcast address
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02


void espnowhandler::esp_now_send_command(ESP_NOW_FOO *pfoo) {
    uint8_t s_data[sizeof(ESP_NOW_FOO)];
    memcpy(s_data, pfoo, sizeof(ESP_NOW_FOO));

    esp_now_send(peer_addr, (const uint8_t *) &s_data, sizeof(s_data));
}

/**
 * send blackout command via ESP-NOW
 * the command will be send three times via wireless for more reliability
 *
 * @param blackout true for blackout, false for restore
 */
void espnowhandler::send_blackout(bool blackout) {
    espnow_data.uid = CUBE_ADDR_BCAST;
    espnow_data.command = COMMAND_BLACKOUT;
    espnow_data.commandcounter = ++commandcounter;
    espnow_data.payload[0] = (uint8_t)(blackout ? 0x01 : 0x00);
    espnow_data.payload[1] = 0;
    espnow_data.payload[2] = 0;
    espnow_data.payload[3] = 0;

    esp_now_send_command(&espnow_data);
    delay(20);
    espnow_data.commandcounter = ++commandcounter;
    esp_now_send_command(&espnow_data);
    delay(20);
    espnow_data.commandcounter = ++commandcounter;
    esp_now_send_command(&espnow_data);
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
    espnow_data.uid = uid;
    espnow_data.command = COMMAND_COLOR;
    espnow_data.commandcounter = ++commandcounter;
    espnow_data.payload[0] = fadetime;
    espnow_data.payload[1] = red;
    espnow_data.payload[2] = green;
    espnow_data.payload[3] = blue;

    esp_now_send_command(&espnow_data);
#ifdef UART_DEBUG
    Serial.printf("send color %d,%d,%d,%d\n", espnow_data.payload[0],espnow_data.payload[1],espnow_data.payload[2],espnow_data.payload[3]);
#endif
}


void espnowhandler::init() {
    espnow_data.preamble = PREAMBLE;
    espnow_data.command = COMMAND_COLOR;
    espnow_data.crc = 0; //TODO: implement this
    espnow_data.commandcounter = 0;


    if (esp_now_init()!=0) {
        Serial.println("ESP-NOW not initialised...");
        ESP.restart();
        delay(1);
    }

    // fill esp now peer buffer with 0xff since we will send everything on esp-now broadcast
    for (int ii = 0; ii < 6; ++ii) {
        peer_addr[ii] = (uint8_t)0xff;
    }

    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(&peer);
}
