# esp32_cubes Gateway

Firmware based on PlatformIO for ESP32 based gateway device to control RGB lights
via ESP-NOW protocol.

Currently this is used to control the colors of 50 plastic LED Cubes (30x30x30cm)
which are Battery powered. They were retro-fitted with ESP32 Modules.

This Project consists of the following parts:

 * ESP32 as Gateway (this project folder)
 * Processing.Org Control Software (../processing/)
 * ESP32 for the Lights (../firmware/) 
 

For information about the protocol used with ESP-NOW in the project, please see the
File "PROTOCOL.md" one folder up.


## Control via Computer (USB-Serial)

Connect the Gateway-ESP32 via USB to you computer and send the following command String
to the ESP with a refresh rate of max. 50Hz

Serial Settings: 115200 Baud, 8N1

### Serial Protocol

String based protocol: CMD,PAYLOAD

All Items are comma separated. Line must end with "X".

Commands (CMD):

  * A = "Blackout all"\
    Payload = 1 Byte (0 = blackout, 1 = restore)
  * B = "Set Color"\
    Payload = 5 Bytes (ADDR,FADETIME,RED,GREEN,BLUE)
  * C = "Set default color"\
    Payload = 1 byte (ADDR)
    

#### Blackout:

This will blackout all lights immediately. The lights will store their last set
color for restore. This function internally uses the Broadcast Address (0xff) to
send the command at the same time to all lights.

Send one Char/Byte (uint8_t) Payload, where

  * 0 = blackout
  * 1 = restore last shown color 
    
Example Java Processing code:
```java
Serial gateway;
gateway = new Serial(this, "/dev/ttyUSB0", 115200);
gateway.write("A,1" + 'X');
```
    
##### Set Color:

This will set a color of an individual light. Available parameters are fading
time and the values for RGB as Bytes.

Fadetime is in Centiseconds, so a Value of 100 will be 1s. Values are from
0 to 255 (0s to 2,55s)

Values for R,G and B are from 0 to 255 (zero to max brightness).
  
Example Java Processing code:
```java
// send purple color to light number 31, fade time 100 ms
Serial gateway;
gateway = new Serial(this, "/dev/ttyUSB0", 115200);
gateway.write("B,31,10,200,0,100" + 'X');
```    

##### Set Default Color

This command will send a control command to the light and it will store its current
color as default color. This is useful to use the lights without a gateway in a static
color.

Example Java Processing code:
```java
// send default color command to light number 31
Serial gateway;
gateway = new Serial(this, "/dev/ttyUSB0", 115200);
gateway.write("C,31" + 'X');
```    

## Control via Art-Net

The project supports the Olimex ESP32-Gateway (and with minor changes many other) Board.

Upload the Ethernet Firmware variant:

    pio run -e esp32-gateway -t upload

The device will get an IP via DHCP and listens on Art-Net Universe 5.

DMX configuration (Fixture Definition):
Channel 1: red, Channel 2: green, Channel 3: blue, Channel 4: control


| DMX Channel | Cube UID | Function    |
|-------------|----------|-------------|
| 1           | 1        | red 0-255   |
| 2           | 1        | green 0-255 |
| 3           | 1        | blue 0-255  |
| 4           | 1        | control     |
| 5           | 2        | red 0-255   |
| 6           | 2        | green 0-255 |
| 7           | 2        | blue 0-255  |
| 8           | 2        | control     |
| ....        | ...      | ....        |
| 121         | 31       | red 0-255   |
| 122         | 31       | green 0-255 |
| 123         | 31       | blue 0-255  |
| 124         | 31       | control     |
| ....        | ...      | ....        |

The Gateway supports maximum 50 cubes and is configured to send the Data for 50. You can however use less without any change in code.
