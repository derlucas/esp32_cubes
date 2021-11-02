# esp32_cubes LED-Cube Firmware

Firmware based on PlatformIO for ESP32 based Light device to control RGB lights
via ESP-NOW protocol.

Currently this is used to control the colors of 50 plastic LED Cubes (30x30x30cm)
which are Battery powered. They were retro-fitted with ESP32 Modules.

This Project consists of the following parts:

 * ESP32 as Gateway (../gateway_firmware/)
 * Processing.Org Control Software (../processing/) (obsolete, now Art-Net is the new control style)
 * ESP32 for the Lights (this project folder) 
 
The Projects supports "mhetesp32minikit" and "esp32doit-devkit-v1" ESP32 boards.

For information about the protocol used with ESP-NOW in the project, please see the
File "PROTOCOL.md" one folder up.


## Cubes Hardware variants

There are two types of cubes used:

 * GGM-Moebel "Paradiso" with 10xRGB LEDs
 * cheap ebay Model with just 3xRGB LEDs

Most of the time we used the "esp32doit" board. Only for testing the
"mhetesp32minikit" was used in the cubes itself.

In the "Paradiso" Cubes, we just replaced the µC with a ESP32.
The LED were simple RGB leds with some transistors.

In the cheap Ebay Cubes, we had to replace the µC and the LEDs.
The LEDs were replaced by two 10cm Strips of WS2812b. Only the charging 
circuit was left intact. 


# Function of this Firmware

The Board just receives Data via ESP-NOW and based on the hardware variant it
outputs PWM signals or WS2812b Data to the LEDs.

## OTA Update

On each boot of the ESP32 the Device will look after a WiFi SSID
"esp32-wuerfel-ota" with the password "geheim323232". If it finds this and can
connect to this network, it will enter a OTA Mode (Over The Air update).
It will print its IPv4 Address on the Serial console. You can now use the PlatformIO
or Arduino OTA function to Upload a new Firmware.

If the OTA WiFi was not found, a normal Boot will continue.
Before trying the OTA WiFi the LED will display yellow. When connected to OTA WiFi
the LEDs will be flashing blue. The Device will wait 10 Seconds to have an OTA.
After that time, it will continue to boot normal. Boot complete is indicated with a
rainbow color effect.

 - set up the OTA wifi with DHCP server
 - monitor your router/DHCP Server for new leases
 - run
 
   pio run -e esp32doit_pwm_ota -t upload --upload-port 192.168.7.66
   
   within 10 seconds (blinking blue)


## the Uniqe ID

When first powered on after flashing the Firmware, connect via Serial Monitor.
You will be prompted to enter the Light id "uid". Enter a value between 1 and
50 (the Processing Software will use 1 to 50) to give your light a uniqe ID.
From now on, you can press "c" on the Serial console at boot to remove the UID
and enter a new one.

Each cube uses 4 Channels (R,G,B,Control) starting at Channel ( $UID - 1 ) * 4 + 1.

