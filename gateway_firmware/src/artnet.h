#ifndef GATEWAY_FIRMWARE_ARTNET_H
#define GATEWAY_FIRMWARE_ARTNET_H

#include <cstdint>
#include <array>
#include <vector>
#include <functional>
#include <libcoap.h>

class artnet {
    // Packet Summary : https://art-net.org.uk/structure/packet-summary-2/
    // Packet Definition : https://art-net.org.uk/structure/streaming-packets/artdmx-packet-definition/

    // UDP specific
#define ART_NET_PORT 6454
// Opcodes
#define ART_POLL 0x2000
#define ART_POLL_REPLY 0x2100
#define ART_DMX 0x5000
#define ART_SYNC 0x5200
// Buffers
#define MAX_BUFFER_ARTNET 530
// Packet
#define ART_NET_ID "Art-Net\0"
#define ART_DMX_START 18

#define DMX_SIZE    512


public:
    typedef struct  {
        uint8_t id[8];
        uint16_t opCode;
        uint8_t ip[4];
        uint16_t port;
        uint8_t verH;
        uint8_t ver;
        uint8_t subH;
        uint8_t sub;
        uint8_t oemH;
        uint8_t oem;
        uint8_t ubea;
        uint8_t status;
        uint8_t etsaman[2];
        uint8_t shortname[18];
        uint8_t longname[64];
        uint8_t nodereport[64];
        uint8_t numbportsH;
        uint8_t numbports;
        uint8_t porttypes[4];//max of 4 ports per node
        uint8_t goodinput[4];
        uint8_t goodoutput[4];
        uint8_t swin[4];
        uint8_t swout[4];
        uint8_t swvideo;
        uint8_t swmacro;
        uint8_t swremote;
        uint8_t sp1;
        uint8_t sp2;
        uint8_t sp3;
        uint8_t style;
        uint8_t mac[6];
        uint8_t bindip[4];
        uint8_t bindindex;
        uint8_t status2;
        uint8_t filler[26];
    } __attribute__((packed)) artnet_reply_s;


    static uint16_t parse_udp_buffer(const char * buffer, int len, sockaddr_in *source);
    static void init_poll_reply();

    static artnet_reply_s * get_reply();
    static uint8_t * get_dmx_data();


private:
//    static uint8_t  node_ip_address[4];
//    static uint8_t  id[8];

    static artnet_reply_s artpoll_reply_package;

//    static uint8_t artnetPacket[MAX_BUFFER_ARTNET];
//    static uint16_t packetSize;
//    static uint16_t opcode;
//    static uint8_t sequence;
//    static uint16_t incomingUniverse;
//    static uint16_t dmxDataLength;
//
//    static uint8_t dmxData[DMX_SIZE];


};


#endif //GATEWAY_FIRMWARE_ARTNET_H
