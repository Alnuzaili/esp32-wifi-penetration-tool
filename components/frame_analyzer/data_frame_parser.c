#include "data_frame_parser.h"

#include <stdint.h>
#include "arpa/inet.h"

#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#include "esp_log.h"
#include "esp_wifi_types.h"

#include "data_frame_types.h"

const char *TAG = "frame_analyzer:data_frame_parser";

ESP_EVENT_DEFINE_BASE(DATA_FRAME_EVENTS);

void print_raw_frame(wifi_promiscuous_pkt_t *frame){
    for(unsigned i = 0; i < frame->rx_ctrl.sig_len; i++) {
        printf("%02x", frame->payload[i]);
    }
    printf("\n");
}

void print_mac_address(uint8_t *a){
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
    a[0], a[1], a[2], a[3], a[4], a[5]);
}

// returns NULL if no EAPOL packet found, otherwise returns pointer to whole raw frame
eapol_packet_t *parse_eapol_packet(wifi_promiscuous_pkt_t *frame) {
    uint8_t *frame_buffer = frame->payload;

    data_frame_mac_header_t *mac_header = (data_frame_mac_header_t *) frame_buffer;
    frame_buffer += sizeof(data_frame_mac_header_t);

    // TODO only for debug purposes
    if((mac_header->addr1[0] != 0x04) && (mac_header->addr2[0] != 0x04)) {
        return NULL;
    }

    if(mac_header->frame_control.protected_frame == 1) {
        ESP_LOGV(TAG, "Protected frame, skipping...");
        return NULL;
    }

    if(mac_header->frame_control.subtype > 7) {
        ESP_LOGD(TAG, "QoS data frame");
        // Skipping QoS field (2 bytes)
        frame_buffer += 2;
    }

    // Skipping LLC SNAP header (6 bytes)
    frame_buffer += sizeof(llc_snap_header_t);

    // Check if frame is type of EAPoL
    if(ntohs(*(uint16_t *) frame_buffer) == ETHER_TYPE_EAPOL) {
        ESP_LOGD(TAG, "EAPOL packet");
        frame_buffer += 2;
        eapol_packet_t *eapol_packet = (eapol_packet_t *) frame_buffer; 
        if(eapol_packet->header.packet_type == EAPOL_KEY) {
            ESP_LOGD(TAG, "EAPOL-Key");
            print_raw_frame(frame);
            return eapol_packet;
        }
    }
    return NULL;
}

void parse_pmkid_from_eapol_packet(eapol_packet_t *eapol_packet) {
    if(eapol_packet->header.packet_type != EAPOL_KEY){
        ESP_LOGE(TAG, "Not an EAPoL-Key packet!");
        return;
    }
    eapol_key_packet_t *eapol_key = (eapol_key_packet_t *) eapol_packet->packet_body;

}