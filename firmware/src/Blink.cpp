#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <Preferences.h>

//#define UART_DEBUG

#define OTA_WIFI_SSID       "esp32-wuerfel-ota"
#define OTA_WIFI_PASSWORD   "geheim323232"
#define OTA_WAIT_TIMEOUT    100     // in 0.1s increments -> 10s

#define PREAMBLE            0xAABB
#define CUBE_ADDR_BCAST     0xff
#define COMMAND_BLACKOUT    0x01
#define COMMAND_COLOR       0x02

#ifdef USE_WS2812
#define WS2812_PIN          14
#define WS2812_COUNT        14
#endif

// on Boards without WS2812 we use this PINout and Gamma Tables
#ifdef USE_PWM
#define LED_PIN_WHITE   14
#define LED_PIN_BLUE    25
#define LED_PIN_RED     26
#define LED_PIN_GREEN   27

const uint8_t gamma8[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
        1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
        2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
        5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
       10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
       17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
       25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
       37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
       51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
       69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
       90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
      115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
      144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
      177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
      215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
const uint8_t gamma_green[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,  4,  4,
    4,  4,  5,  5,  5,  5,  6,  6,  6,  7,  7,  7,  8,  8,  8,  9,
    9,  9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16,
   16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 24,
   25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32, 33, 34, 35, 35,
   36, 37, 38, 38, 39, 40, 41, 42, 42, 43, 44, 45, 46, 47, 47, 48,
   49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 61, 62, 63,
   64, 65, 66, 67, 68, 69, 70, 71, 73, 74, 75, 76, 77, 78, 79, 80,
   81, 82, 84, 85, 86, 87, 88, 89, 91, 92, 93, 94, 95, 97, 98, 99,
  100,102,103,104,105,107,108,109,111,112,113,115,116,117,119,120,
  121,123,124,126,127,128,130,131,133,134,136,137,139,140,142,143,
  145,146,148,149,151,152,154,155,157,158,160,162,163,165,166,168,
  170,171,173,175,176,178,180,181,183,185,186,188,190,192,193,195,
  197,199,200,202,204,206,207,209,211,213,215,217,218,220,222,224,
  226,228,230,232,233,235,237,239,241,243,245,247,249,251,253,255 };
#endif

uint32_t commandcounter = 0;    // this is currently not used.

struct ESP_NOW_FOO {
    uint16_t preable;
    uint8_t uid;
    uint32_t commandcounter;
    uint8_t command;
    uint8_t crc;
    uint8_t payload[4];
};

void handle_command_color(const ESP_NOW_FOO &command);

void handle_command_blackout(const ESP_NOW_FOO &command);

enum OTA_MODE {
    NONE,
    SEARCHING,
    WAITING,
    UPDATING
} otaMode = NONE;

uint16_t otaWaitCounter;
uint16_t uid;
Preferences prefs;
boolean startupFadeDone = false;
boolean blackedout = false;

#ifdef USE_WS2812
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(WS2812_COUNT, WS2812_PIN);
#endif

// we use Animations and RgbColor from NeoPixelBus also for our PWM implementation
NeoPixelAnimator animations(1, NEO_CENTISECONDS);
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor black(0);
RgbColor currentSetColor;

#ifdef USE_PWM
void initPWM() {
    pinMode(LED_PIN_RED, OUTPUT);
    pinMode(LED_PIN_GREEN, OUTPUT);
    pinMode(LED_PIN_BLUE, OUTPUT);
    pinMode(LED_PIN_WHITE, OUTPUT);

    digitalWrite(LED_PIN_RED, LOW);
    digitalWrite(LED_PIN_GREEN, LOW);
    digitalWrite(LED_PIN_BLUE, LOW);
    digitalWrite(LED_PIN_WHITE, LOW);

    ledcAttachPin(LED_PIN_RED, 1);
    ledcAttachPin(LED_PIN_GREEN, 2);
    ledcAttachPin(LED_PIN_BLUE, 3);
    ledcAttachPin(LED_PIN_WHITE, 4);

    ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
    ledcSetup(2, 12000, 8);
    ledcSetup(3, 12000, 8);
    ledcSetup(4, 12000, 8);

    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    ledcWrite(4, 0);
}
#endif


void blackout() {
    animations.StopAll();

#ifdef USE_WS2812
    for (uint16_t pixel = 0; pixel < WS2812_COUNT; pixel++) {
        strip.SetPixelColor(pixel, black);
    }
    strip.Show();
#endif
#ifdef USE_PWM
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    ledcWrite(4, 0);
#endif

}

/**
 * Sets a color instantaneously
 *
 * @param color The color to display
 */
void setColor(RgbColor color) {
#ifdef USE_WS2812
    for (uint16_t pixel = 0; pixel < WS2812_COUNT; pixel++) {
        strip.SetPixelColor(pixel, color);
    }

    strip.Show();
#endif
#ifdef USE_PWM
    ledcWrite(1, gamma8[(uint8_t)((float)color.R * 0.9)]);
    ledcWrite(2, gamma8[color.G]);
    ledcWrite(3, gamma8[(uint8_t)((float)color.B * 0.9)]);
#endif
    currentSetColor = color;
}

/**
 * Sets a color fade the output to
 *
 * @param duration      The fade duration in centiseconds.
 * @param targetColor   The color to fade to.
 */
void fadeTo(uint8_t duration, RgbColor targetColor) {

    animations.StopAll();

    RgbColor startColor = currentSetColor;

    AnimUpdateCallback animUpdate = [=](const AnimationParam &param) {
        setColor(RgbColor::LinearBlend(startColor, targetColor, param.progress));
    };

    animations.StartAnimation(0, duration, animUpdate);
}

/**
 * This is a short startup animation to show the user the device is powered on
 * It will be a fade from Black to Red to Green to Blue and Black
 */
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
            // this way we check if the startup is done and then set a dim purple solid color
            startupFadeDone = true;
            setColor(RgbColor(20, 0, 20));
        }
    };

    animations.StartAnimation(0, 100, animUpdate);
}

/**
 * this function is called on ESP-Receive
 *
 * @param mac_addr
 * @param data
 * @param len
 */
void on_receive_data(const uint8_t *mac_addr, const uint8_t *data, int len) {
#ifdef UART_DEBUG
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Recv from: ");
    Serial.print(macStr);
    Serial.printf(" len: %d\n", len);
#endif

    if (!startupFadeDone) {
        return;
    }

    ESP_NOW_FOO command;

    if (len == sizeof(command)) {
        memcpy(&command, data, sizeof(command));

        if (command.preable != PREAMBLE) {
#ifdef UART_DEBUG
            Serial.println("wrong preamble");
#endif
            return;
        }

#ifdef UART_DEBUG
        Serial.printf("command: %d, uid: %d\n", command.command, command.uid);
#endif

        if (command.uid == CUBE_ADDR_BCAST || command.uid == uid) {
            if (command.command == COMMAND_COLOR && !blackedout) {
                handle_command_color(command);
            } else if (command.command == COMMAND_BLACKOUT) {
                handle_command_blackout(command);
            }
        }

    }
}

void handle_command_color(const ESP_NOW_FOO &command) {
#ifdef UART_DEBUG
    Serial.printf("color command %d %d %d %d\n",command.payload[0], command.payload[1], command.payload[2], command.payload[3]);
#endif

    animations.StopAll();

    if (command.payload[0] > 0) {
        // fade
        fadeTo(command.payload[0], RgbColor(command.payload[1], command.payload[2], command.payload[3]));

    } else {
        // switch color directly
        setColor(RgbColor(command.payload[1], command.payload[2], command.payload[3]));
    }
}

void handle_command_blackout(const ESP_NOW_FOO &command) {
#ifdef UART_DEBUG
    Serial.println("blackout command");
#endif

    if (command.payload[0] == 0x01) {
#ifdef UART_DEBUG
        Serial.println("blacking out");
#endif
        blackout();
        blackedout = true;
    } else {
#ifdef UART_DEBUG
        Serial.println("restoring color");
#endif
        setColor(currentSetColor);
        blackedout = false;
    }
}

/**
 * this function asks on the Serial console to enter a light UID
 * Before the Question it will blink 3 times.
 *
 * @param prefName      The Preference Name to display for Input.
 * @param flashcolor    The color to flash the LEDs
 * @return              The given number.
 */
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

/**
 * this function will set the LED to Yellow color and then
 * try to connect to OTA WiFi.
 */
void checkOTA() {
    otaMode = SEARCHING;
    Serial.println("looking for OTA WiFi...");

    setColor(RgbColor(100,100,0));
    // WARNING: to allow ESP-NOW work, this WiFi must be on Channel 1
    WiFi.begin(OTA_WIFI_SSID, OTA_WIFI_PASSWORD);

    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("no OTA WiFi found, proceed normal boot");
        otaMode = NONE;
        return;
    }

    otaMode = WAITING;
}

void normalBoot() {
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


    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.printf("wifi channel: %d\n", WiFi.channel());

    if (esp_now_init() != 0) {
        Serial.println("err:esp-now");
        ESP.restart();
        delay(1);
    }

    esp_now_register_recv_cb(on_receive_data);
    startupFade();
}

void setup() {

#ifdef USE_PWM
    initPWM();
#endif
#ifdef USE_WS2812
    // this resets all the neopixels to an off state
    strip.Begin();
#endif

    prefs.begin("blink", false);
    Serial.begin(115200);
    Serial.setTimeout(5000);
    while (!Serial); // wait for serial attach

    Serial.println();
    Serial.println("Initializing...");


    checkOTA();
    setColor(black);

    if (otaMode == WAITING) {
        Serial.println("connected to OTA WiFi. Waiting for firmware...");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        ArduinoOTA
                .onStart([]() {
                    otaMode = UPDATING;
                    String type;
                    if (ArduinoOTA.getCommand() == U_FLASH)
                        type = "sketch";
                    else // U_SPIFFS
                        type = "filesystem";

                    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                    Serial.println("Start updating " + type);
                })
                .onEnd([]() {
                    Serial.println("\nEnd");
                })
                .onProgress([](unsigned int progress, unsigned int total) {
                    int prog = (progress / (total / 100));
                    Serial.printf("Progress: %u%%\r", prog);
                    setColor( (prog % 2 == 0) ? blue : red );
                })
                .onError([](ota_error_t error) {
                    Serial.printf("Error[%u]: ", error);
                    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
                    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
                    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                    else if (error == OTA_END_ERROR) Serial.println("End Failed");
                });

        ArduinoOTA.begin();
    } else {
        normalBoot();
    }
}


void loop() {

    if (otaMode != NONE) {
        ArduinoOTA.handle();

        if(otaMode == WAITING) {
            static long mil = millis();
            static boolean huehott = false;

            if(millis() - mil > 100) {
                setColor(huehott ? blue : black);
                huehott = !huehott;
                mil = millis();

                otaWaitCounter++;
                if(otaWaitCounter >= OTA_WAIT_TIMEOUT) {
                    Serial.println("OTA wait timeout, proceeding normal boot");
                    otaMode = NONE;
                    normalBoot();
                }
            }
        }

    } else {

        if (animations.IsAnimating()) {
            animations.UpdateAnimations();

#ifdef USE_WS2812
            strip.Show();
#endif
        }
    }
}

