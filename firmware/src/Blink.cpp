#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Preferences.h>

#define PREAMBLE            0xAABB
#define CUBE_ADDR_BCAST     0xff
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02

uint32_t commandcounter = 0;

struct ESP_NOW_FOO {
    uint16_t preable;
    uint8_t uid;
    uint32_t commandcounter;
    uint8_t command;
    uint8_t crc;
    uint8_t payload[4];
};

const uint16_t PixelCount = 14;
const uint8_t PixelPin = 16;
uint16_t uid, gid;
Preferences prefs;
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);
NeoPixelAnimator animations(PixelCount, NEO_CENTISECONDS);
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor black(0);
boolean startupFadeDone = false;
RgbColor currentSetColor;
boolean blackedout = false;

void blackout() {
    animations.StopAll();
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
        strip.SetPixelColor(pixel, black);
    }
    strip.Show();
}

void setColor(RgbColor color) {
    for (uint16_t pixel = 0; pixel < PixelCount; pixel++) {
        strip.SetPixelColor(pixel, color);
    }
    currentSetColor = color;
    strip.Show();
}

void fadeTo(uint8_t duration, RgbColor targetColor) {

    animations.StopAll();

    RgbColor startColor = currentSetColor;

    AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {

        setColor(RgbColor::LinearBlend(startColor, targetColor, param.progress));
        if (param.progress >= 0.99f) {


        }
    };

    animations.StartAnimation(0, duration, animUpdate);
}

void startupFade() {
    startupFadeDone = false;

    AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {
        float newProgress = param.progress;
        RgbColor sourceColor, targetColor;

        if (param.progress < .25f) {
            sourceColor = black;
            targetColor = red;
            newProgress *= 4;
        } else if (param.progress >= .25f && param.progress < .5f) {
            newProgress -= .25f;
            newProgress *= 4;
            sourceColor = red;
            targetColor = green;
        } else if (param.progress >= .5f && param.progress < .75f) {
            newProgress -= .5f;
            newProgress *= 4;
            sourceColor = green;
            targetColor = blue;
        } else {
            newProgress -= .75f;
            newProgress *= 4;
            sourceColor = blue;
            targetColor = black;
        }

        setColor(RgbColor::LinearBlend(sourceColor, targetColor, newProgress));
        if (param.progress >= 0.99f) {
            startupFadeDone = true;
            setColor(RgbColor(20, 0, 20));
        }
    };

    animations.StartAnimation(0, 100, animUpdate);
}

void on_receive_data(const uint8_t *mac_addr, const uint8_t *data, int len) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
//    Serial.print("Recv from: ");
//    Serial.print(macStr);
//    Serial.printf(" len: %d\n", len);

    if (!startupFadeDone) {
        return;
    }

    ESP_NOW_FOO command;

    if (len == sizeof(command)) {
        memcpy(&command, data, sizeof(command));

        if (command.preable != PREAMBLE) {
            Serial.println("wrong preamble");
            return;
        }
//        Serial.printf("command: %d, uid: %d\n", command.command, command.uid);

        if (command.uid == CUBE_ADDR_BCAST || command.uid == uid) {

            if (command.command == COMMAND_COLOR && !blackedout) {
//                Serial.printf("color command %d %d %d %d\n",command.payload[0], command.payload[1], command.payload[2], command.payload[3]);

                animations.StopAll();

                if (command.payload[0] > 0) {
                    // fade
                    fadeTo(command.payload[0], RgbColor(command.payload[1], command.payload[2], command.payload[3]));

                } else {
                    // switch color directly
                    setColor(RgbColor(command.payload[1], command.payload[2], command.payload[3]));
                }


            } else if (command.command == COMMAND_BLACKOUT) {
//            Serial.println("blackout command");

                if (command.payload[0] == 0x01) {
//                Serial.println("blacking out");
                    blackout();
                    blackedout = true;
                } else {
//                Serial.println("restoring color");
                    setColor(currentSetColor);
                    blackedout = false;
                }

            }
        }

    }
}

uint16_t askNumber(const char *prefName, RgbColor *flashcolor) {

    for (int i = 0; i < 4; ++i) {
        setColor(*flashcolor);
        delay(200);
        setColor(black);
        delay(200);
    }

    bool found = false;

    while (!found) {
        Serial.printf("enter %s: \n", prefName);
        uid = (uint16_t) Serial.parseInt();
        if (uid > 0) {
            Serial.printf("you entered: %d\n", uid);
            found = true;
        }
    }

    return uid;
}


void setup() {
    prefs.begin("blink", false);
    Serial.begin(115200);
    Serial.setTimeout(5000);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.print("MAC:");
    Serial.println(WiFi.macAddress());
    Serial.flush();

    // this resets all the neopixels to an off state
    strip.Begin();
    setColor(black);


    if (esp_now_init() != 0) {
        Serial.println("err:esp-now");
        ESP.restart();
        delay(1);
    }


    uid = prefs.getUShort("uid", 0);

    if (uid != 0) {
        Serial.println("--------------- Enter c to remove UID  -------------");
        setColor(blue);
        Serial.flush();
        delay(1000);
        String inData;

        while (Serial.available() > 0) {
            char received = static_cast<char>(Serial.read());
            inData += received;
            Serial.println(received);

            if (received == 'c' && inData.indexOf("c") > -1) {
                Serial.println("clearing uid");
                prefs.putUShort("uid", 0);
                uid = askNumber("uid", &red);
                prefs.putUShort("uid", uid);
                Serial.printf("my new uid: %d\n", uid);
            }
        }

        setColor(black);

        Serial.printf("my uid: %d\n", uid);
    } else {
        uid = askNumber("uid", &red);
        prefs.putUShort("uid", uid);
        Serial.printf("my new uid: %d\n", uid);
    }

    esp_now_register_recv_cb(on_receive_data);
    startupFade();

}


void loop() {

    if (animations.IsAnimating()) {
        animations.UpdateAnimations();
        strip.Show();
    }

}

