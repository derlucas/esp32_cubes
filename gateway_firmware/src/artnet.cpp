#include <cstring>
#include <lwip/sockets.h>
#include <lwip/ip6_addr.h>
#include <lwip/inet.h>
#include <tcpip_adapter.h>
#include "artnet.h"

#define LOG_LOCAL_LEVEL  3
#include <esp_log.h>
static const char *TAG = "gw-artnet";

artnet::artnet_reply_s artnet::artpoll_reply_package;

uint16_t artnet::parse_udp_buffer(const char *buffer, int len, sockaddr_in *source) {

    //if (len <= MAX_BUFFER_ARTNET && len > 0) {
    if (len > 0) {

        // Check that packetID is "Art-Net" else ignore
        for (uint8_t i = 0 ; i < 8 ; i++) {
            if (buffer[i] != ART_NET_ID[i]) {
                return 0;
            }
        }

        const uint16_t opcode = buffer[8] | (buffer[9] << 8);

        if (opcode == ART_DMX) {

            uint8_t sequence;
            uint16_t incomingUniverse;
            uint16_t dmxDataLength;

            sequence = buffer[12];
            incomingUniverse = buffer[14] | buffer[15] << 8;
            dmxDataLength = buffer[17] | buffer[16] << 8;

            uint8_t dmx[4];
            dmx[0] = (uint8_t)buffer[ART_DMX_START];
            dmx[1] = (uint8_t)buffer[ART_DMX_START+1];
            dmx[2] = (uint8_t)buffer[ART_DMX_START+2];
            dmx[3] = (uint8_t)buffer[ART_DMX_START+3];

            ESP_LOGI(TAG, "DMX seq=%d, universe=%d, len=%d, ch1=%d, ch2=%d, ch3=%d, ch4=%d,", sequence, incomingUniverse, dmxDataLength, dmx[0], dmx[1], dmx[2], dmx[3]);

            //if (artDmxCallback) (*artDmxCallback)(incomingUniverse, dmxDataLength, sequence, artnetPacket + ART_DMX_START, remoteIP);
            //TODO get DMX Data

        } else if (opcode == ART_POLL) {
            //fill the reply struct, and then send it to the network's broadcast address

            ESP_LOGI(TAG, "POLL from: %s", inet_ntoa(source->sin_addr.s_addr));
            tcpip_adapter_ip_info_t ip;
            if(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_ETH, &ip) != ESP_OK) {
                ESP_LOGE(TAG, "error getting own IP");
                return 0;
            }

            uint8_t node_ip_address[4];
            node_ip_address[0] = ip.ip.addr & 0xff;
            node_ip_address[1] = (ip.ip.addr >> 8) & 0xff;;
            node_ip_address[2] = (ip.ip.addr >> 16) & 0xff;;
            node_ip_address[3] = (ip.ip.addr >> 24) & 0xff;

            memcpy(artpoll_reply_package.ip, node_ip_address, sizeof(artpoll_reply_package.ip));

            artpoll_reply_package.bindip[0] = node_ip_address[0];
            artpoll_reply_package.bindip[1] = node_ip_address[1];
            artpoll_reply_package.bindip[2] = node_ip_address[2];
            artpoll_reply_package.bindip[3] = node_ip_address[3];



        } else if (opcode == ART_SYNC) {
//            if (artSyncCallback) (*artSyncCallback)(remoteIP);
        }

        return opcode;
    }

    return 0;
}

artnet::artnet_reply_s *artnet::get_reply() {
    return &artpoll_reply_package;
}

uint8_t *artnet::get_dmx_data() {
    return nullptr;
}

void artnet::init_poll_reply() {
    uint8_t id[8];
    sprintf((char *)id, "Art-Net");
    memcpy(artnet::artpoll_reply_package.id, id, sizeof(artnet::artpoll_reply_package.id));
//    memcpy(artnet::artpoll_reply_package.ip, node_ip_address, sizeof(artnet::artpoll_reply_package.ip));

    artnet::artpoll_reply_package.opCode = ART_POLL_REPLY;
    artnet::artpoll_reply_package.port =  ART_NET_PORT;

    memset(artnet::artpoll_reply_package.goodinput, 0x08, 4);
    memset(artnet::artpoll_reply_package.goodoutput, 0x80, 4);
    memset(artnet::artpoll_reply_package.porttypes, 0xc0, 4);
    

    uint8_t shortname [18];
    uint8_t longname [64];
    memset(shortname, 0x00, sizeof(shortname));
    memset(longname, 0x00, sizeof(longname));

    sprintf((char *)shortname, "artnet arduino");
    sprintf((char *)longname, "Art-Net -> Arduino Bridge");
    memcpy(artnet::artpoll_reply_package.shortname, shortname, sizeof(shortname));
    memcpy(artnet::artpoll_reply_package.longname, longname, sizeof(longname));

    artnet::artpoll_reply_package.etsaman[0] = 0;
    artnet::artpoll_reply_package.etsaman[1] = 0;
    artnet::artpoll_reply_package.verH       = 1;
    artnet::artpoll_reply_package.ver        = 0;
    artnet::artpoll_reply_package.subH       = 0;
    artnet::artpoll_reply_package.sub        = 0;
    artnet::artpoll_reply_package.oemH       = 0;
    artnet::artpoll_reply_package.oem        = 0xFF;
    artnet::artpoll_reply_package.ubea       = 0;
    artnet::artpoll_reply_package.status     = 0xd2;
    artnet::artpoll_reply_package.swvideo    = 0;
    artnet::artpoll_reply_package.swmacro    = 0;
    artnet::artpoll_reply_package.swremote   = 0;
    artnet::artpoll_reply_package.style      = 0;

    artnet::artpoll_reply_package.numbportsH = 0;
    artnet::artpoll_reply_package.numbports  = 4;
    artnet::artpoll_reply_package.status2    = 0x08;

//    artnet::artpoll_reply_package.bindip[0] = node_ip_address[0];
//    artnet::artpoll_reply_package.bindip[1] = node_ip_address[1];
//    artnet::artpoll_reply_package.bindip[2] = node_ip_address[2];
//    artnet::artpoll_reply_package.bindip[3] = node_ip_address[3];

    uint8_t swin[4]  = {0x01,0x02,0x03,0x04};
    uint8_t swout[4] = {0x01,0x02,0x03,0x04};
    for(uint8_t i = 0; i < 4; i++)
    {
        artnet::artpoll_reply_package.swout[i] = swout[i];
        artnet::artpoll_reply_package.swin[i] = swin[i];
    }

    sprintf((char *)artnet::artpoll_reply_package.nodereport, "%i DMX output universes active.", artnet::artpoll_reply_package.numbports);
}

//void artnet::printPacketHeader()
//{
//    Serial.print("packet size = ");
//    Serial.print(packetSize);
//    Serial.print("\topcode = ");
//    Serial.print(opcode, HEX);
//    Serial.print("\tuniverse number = ");
//    Serial.print(incomingUniverse);
//    Serial.print("\tdata length = ");
//    Serial.print(dmxDataLength);
//    Serial.print("\tsequence n0. = ");
//    Serial.println(sequence);
//}
//
//void artnet::printPacketContent()
//{
//    for (uint16_t i = ART_DMX_START ; i < dmxDataLength ; i++){
//        Serial.print(artnetPacket[i], DEC);
//        Serial.print("  ");
//    }
//    Serial.println('\n');
//}