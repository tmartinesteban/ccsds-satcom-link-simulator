#include "ccsds_frame.h"
#include <string.h>

uint16_t ccsds_calculate_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF; /* Valor inicial estándar CCSDS */
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void ccsds_build_frame(CCSDS_TelemetryFrame *frame, uint16_t sc_id, uint16_t counter, const uint8_t *payload_data) {
    frame->sync_marker = CCSDS_ASM_SYNC_MARKER;
    frame->spacecraft_id = sc_id;
    frame->frame_counter = counter;
    memcpy(frame->payload, payload_data, CCSDS_PAYLOAD_SIZE);

    /* El CRC se calcula sobre cabecera + payload (excluyendo sync_marker y el propio crc16) */
    const uint8_t *crc_start = (const uint8_t *)&frame->spacecraft_id;
    size_t crc_len = sizeof(frame->spacecraft_id) + sizeof(frame->frame_counter) + CCSDS_PAYLOAD_SIZE;
    frame->crc16 = ccsds_calculate_crc16(crc_start, crc_len);
}

int ccsds_verify_frame(const CCSDS_TelemetryFrame *frame) {
    if (frame->sync_marker != CCSDS_ASM_SYNC_MARKER) {
        return 0; /* Pérdida de sincronismo */
    }
    const uint8_t *crc_start = (const uint8_t *)&frame->spacecraft_id;
    size_t crc_len = sizeof(frame->spacecraft_id) + sizeof(frame->frame_counter) + CCSDS_PAYLOAD_SIZE;
    uint16_t expected_crc = ccsds_calculate_crc16(crc_start, crc_len);
    return (frame->crc16 == expected_crc);
}