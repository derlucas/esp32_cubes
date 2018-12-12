#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#define PREAMBLE            0xAABB
#define CUBE_START_ID       1
#define CUBE_COUNT          50
#define CUBE_ADDR_BCAST     0xff
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02

uint8_t peer_addr[6];
uint32_t commandcounter = 0;

struct ESP_NOW_FOO {
    uint16_t preable = PREAMBLE;
    uint8_t uid;
    uint32_t commandcounter = 0;
    uint8_t command;
    uint8_t crc;
    uint8_t payload[4];
};

struct LED_COMMAND {
    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    uint8_t centiseconds = 0;
};

void esp_now_send_command(ESP_NOW_FOO *pfoo) {
    uint8_t s_data[sizeof(ESP_NOW_FOO)];
    memcpy(s_data, pfoo, sizeof(ESP_NOW_FOO));

    esp_now_send(peer_addr, (const uint8_t *) &s_data, sizeof(s_data));
}

void send_blackout(bool blackout) {

    ESP_NOW_FOO foo;
    foo.uid = CUBE_ADDR_BCAST;
    foo.command = COMMAND_BLACKOUT;
    foo.commandcounter = ++commandcounter;
    foo.payload[0] = (uint8_t)(blackout ? 0x01 : 0x00);
    foo.payload[1] = 0; foo.payload[2] = 0; foo.payload[3] = 0;

    esp_now_send_command(&foo);
    delay(20);
    foo.commandcounter = ++commandcounter;
    esp_now_send_command(&foo);
    delay(20);
    foo.commandcounter = ++commandcounter;
    esp_now_send_command(&foo);
//    Serial.printf("send blackout %d\n", foo.payload[0]);
}

void send_color(uint8_t uid, uint8_t fadetime, uint8_t red, uint8_t green, uint8_t blue) {
    ESP_NOW_FOO foo;
    foo.uid = uid;
    foo.command = COMMAND_COLOR;
    foo.commandcounter = ++commandcounter;
    foo.payload[0] = fadetime;
    foo.payload[1] = red;
    foo.payload[2] = green;
    foo.payload[3] = blue;

    esp_now_send_command(&foo);
//    Serial.printf("send color %d,%d,%d,%d\n", foo.payload[0],foo.payload[1],foo.payload[2],foo.payload[3]);
}

void setup() {
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing Gateway...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.printf("wifi channel: %d\n", WiFi.channel());



    if (esp_now_init()!=0) {
        Serial.println("Protokoll ESP-NOW nicht initialisiert...");
        ESP.restart();
        delay(1);
    }

    for (int ii = 0; ii < 6; ++ii) {
        peer_addr[ii] = (uint8_t)0xff;
    }


    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    esp_now_add_peer(&peer);

}




void loop() {


    // Serial   COMMAND,ID,PAYLOAD\n
    // Commands:
    // A = Blackout alles           Payload = 1 byte  (0 = blackout, 1 = restore)
    // B = Farbe Setzen             Payload = 5 bytes (uid,FADETIME,ROT,GRUEN,BLAU )
    //
    //
    // B,255,0,100,0,0
    // B,255,0,0,100,0
    // B,255,200,0,0,100
    // B,255,200,0,200,100
    // B,2,0,100,0,0
    //

    String inData;

    while (Serial.available() > 0) {
        inData = Serial.readStringUntil('\n');

//        Serial.print("Received: ");
//        Serial.println(inData);

        if(inData.startsWith("A,")) {
            if(inData.startsWith("A,0")) {
                send_blackout(false);
            } else if(inData.startsWith("A,1")) {
                send_blackout(true);
            }
        } else if(inData.startsWith("B,")) {
            uint8_t uid,fadetime,red,green,blue, count;
            count = sscanf(inData.c_str(), "B,%d,%d,%d,%d,%d", &uid, &fadetime, &red, &green, &blue);

//            Serial.printf("found %d\n", count);
//            Serial.printf("i,t,r,g,b: %d, %d, %d, %d, %d\n", uid, fadetime, red, green, blue);
            send_color(uid, fadetime, red, green, blue);
        }


        inData = "";
    }
}

/*
 * Protocol
 * preamble, dst-id/bcast, function, payload, commandcounter
 *
 * Broadcast = uid 0xff
 *
 * Funktionen:
 *  - Blackout Broadcast (alle sofort aus)
 *  - Farbe Broadcast setzen
 *  - Farbe Broadcast fadeTo (ms)
 *  - Farbe Cube setzen
 *  - Farbe Cube fadeTo (ms)q
 *  - Helligkeit Broadcast
 *  - Helligkeit Cube
 *
 *
 *
 *
 *
 *
 *
 */