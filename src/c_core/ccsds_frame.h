#ifndef CCSDS_FRAME_H
#define CCSDS_FRAME_H

#include <stdint.h>
#include <stddef.h>

/* Marcador de Sincronismo Estándar CCSDS (Attached Sync Marker - 32 bits) */
#define CCSDS_ASM_SYNC_MARKER 0x1ACFFC1D
#define CCSDS_PAYLOAD_SIZE    16

/* Estructura empaquetada sin padding de memoria (crucial en telecomunicaciones) */
#pragma pack(push, 1)
typedef struct {
    uint32_t sync_marker;                  /* 4 bytes: 0x1ACFFC1D */
    uint16_t spacecraft_id;                /* 2 bytes: ID del satélite */
    uint16_t frame_counter;                /* 2 bytes: Contador secuencial de trama */
    uint8_t  payload[CCSDS_PAYLOAD_SIZE];  /* 16 bytes: Datos de sensores (AOCS, temp, batería) */
    uint16_t crc16;                        /* 2 bytes: CRC-16-CCITT para detección de errores */
} CCSDS_TelemetryFrame;
#pragma pack(pop)

/* Prototipos de funciones */
uint16_t ccsds_calculate_crc16(const uint8_t *data, size_t length);
void ccsds_build_frame(CCSDS_TelemetryFrame *frame, uint16_t sc_id, uint16_t counter, const uint8_t *payload_data);
int ccsds_verify_frame(const CCSDS_TelemetryFrame *frame);

#endif /* CCSDS_FRAME_H */