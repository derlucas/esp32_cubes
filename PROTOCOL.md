# ESP32-Wuerfel ESP-NOW Protocol

The following simple protocol is used to send data via Wireless Interface from the Gateway to the lights:

 * Preamble
 * Address
 * Command Counter
 * Function
 * CRC
 * Payload


| Field    | Size                | Description                                      |
|----------|---------------------|--------------------------------------------------|
| Preamble | 2 Bytes (uint16)    | Fixed Preamble (must always be 0xAABB)           |
| Address  | 1 Byte (uint8)      | Address of Light, 0 to 254, 255 for Broadcast    |
| Counter  | 4 Bytes (uint32)    | A counter which must be incremented on each send |
| Function | 1 Byte (uint8)      | Function selector, see Table below               |
| CRC      | 1 Byte (uint8)      | Currently not used Checksum field                |
| Payload  | 4*50 Bytes (uint8)  | Payload depends on Function, size is always 4*50 |


The following C Struct will be send as it is via the ESP32 function "esp_now_send()":
```C
struct ESP_NOW_FOO {
    uint16_t preamble;
    uint8_t uid;
    uint32_t commandcounter;
    uint8_t command;
    uint8_t crc;
    uint8_t payload[4*50];
};
```

## Functions

 * COMMAND_BLACKOUT = 0x01
 * COMMAND_COLOR = 0x02
 * COMMAND_DEF_COLOR = 0x03
 * COMMAND_COLOR_BCAST = 0x04
 
 
## Blackout:

 * Payload[0] = 0 = blackout / 1 = restore last shown color
 * Payload[1-3] = don't care (send zeros) 
        
## Set Color:

 * Payload[0] = Fade Time in Centiseconds
 * Payload[1] = Color Red
 * Payload[2] = Color Green
 * Payload[3] = Color Blue
 
## Set Default Color:

The light will store its current color in memory and show this color on boot.
To disable the default color, set color to black and send this command again.

 * Payload[0] = Set to 0x01 to store current color
 * Payload[1-3] = don't care (send zeros) 


## Art-Net Control / New Protocol changes

Now you can use a Gateway with Ethernet Connection and send Art-Net DMX Data to 
control the Lights. For this feature, there is the new command "COMMAND_COLOR_BCAST" and the payload buffer was extended to 200 Bytes to contain 50x4 Bytes. So each Device has 4 Bytes of data (RED,GREEN,BLUE,Control). The idea is to send all the 200 Bytes to the broadcast Address via ESP-NOW and save Airtime of the WiFi. Each cube will take only the 4 Bytes based of their UID.

There is no fadetime in this control mode, since you most likely will use a lighting control software with build in color mixing and effects built in.

### Control Channel

Currently the Control channel is used as following:

| DMX Value | Function |
| ----------|----------|
| 0 - 249   | unused   |
| 250 - 255 | store current color as boot color |
