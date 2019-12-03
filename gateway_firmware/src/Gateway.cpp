#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ETH.h>
#include "espnowhandler.h"

//#define UART_DEBUG
#define USE_ARTNET

espnowhandler handler;


void setup() {
    Serial.begin(115200);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing Gateway...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.printf("wifi channel: %d\n", WiFi.channel());    // we use default WiFi channel (other might not work)

    handler.init();
}



void loop() {

    String inData;

    while (Serial.available() > 0) {
        inData = Serial.readStringUntil('\n');

#ifdef UART_DEBUG
        Serial.print("Received: ");
        Serial.println(inData);
#endif

        if(inData.startsWith("A,")) {
            if(inData.startsWith("A,0")) {
                handler.send_blackout(false);
            } else if(inData.startsWith("A,1")) {
                handler.send_blackout(true);
            }
        } else if(inData.startsWith("B,")) {
            int uid,fadetime,red,green,blue,count;
            count = sscanf(inData.c_str(), "B,%d,%d,%d,%d,%d", &uid, &fadetime, &red, &green, &blue);

#ifdef UART_DEBUG
            Serial.printf("found %d\n", count);
            Serial.printf("i,t,r,g,b: %d, %d, %d, %d, %d\n", uid, fadetime, red, green, blue);
#endif

            handler.send_color(uid, fadetime, red, green, blue);
        }

        inData = "";
    }
}

