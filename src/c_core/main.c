#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ccsds_frame.h"
#include "modulator.h"

#define NUM_FRAMES 200

int main(void) {
    FILE *fp = fopen("build/tx_iq.csv", "w");
    if (!fp) {
        perror("Error abriendo build/tx_iq.csv");
        return 1;
    }
    fprintf(fp, "frame_id,sym_idx,I,Q\n");

    CCSDS_TelemetryFrame tx_frame;
    uint8_t payload[CCSDS_PAYLOAD_SIZE];
    IQSample symbols[sizeof(CCSDS_TelemetryFrame) * 8];
    uint8_t rx_bytes[sizeof(CCSDS_TelemetryFrame)];

    for (uint16_t f = 0; f < NUM_FRAMES; f++) {
        for (size_t k = 0; k < CCSDS_PAYLOAD_SIZE; k++) {
            payload[k] = (uint8_t)((f + k * 17) & 0xFF);
        }

        ccsds_build_frame(&tx_frame, 0x042A, f, payload);
        size_t n_sym = modulate_bytes((uint8_t *)&tx_frame, sizeof(CCSDS_TelemetryFrame), MOD_QPSK, symbols);

        /* Verificación loopback sin ruido */
        demodulate_symbols(symbols, n_sym, MOD_QPSK, rx_bytes);
        CCSDS_TelemetryFrame *rx_frame = (CCSDS_TelemetryFrame *)rx_bytes;
        if (!ccsds_verify_frame(rx_frame)) {
            fprintf(stderr, "Fallo de loopback en trama %u\n", f);
            fclose(fp);
            return 1;
        }

        for (size_t s = 0; s < n_sym; s++) {
            fprintf(fp, "%u,%zu,%.6f,%.6f\n", f, s, symbols[s].i, symbols[s].q);
        }
    }

    fclose(fp);
    printf("OK: %d tramas CCSDS (%zu bytes/trama) moduladas en QPSK -> build/tx_iq.csv\n",
           NUM_FRAMES, sizeof(CCSDS_TelemetryFrame));
    return 0;
}