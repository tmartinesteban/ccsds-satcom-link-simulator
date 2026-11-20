#include "modulator.h"
#include <math.h>

#define INV_SQRT2 0.70710678118654752440

size_t modulate_bytes(const uint8_t *bytes, size_t num_bytes, ModulationType mod, IQSample *out_symbols) {
    size_t sym_idx = 0;

    if (mod == MOD_BPSK) {
        for (size_t b = 0; b < num_bytes; b++) {
            for (int bit = 7; bit >= 0; bit--) {
                uint8_t val = (bytes[b] >> bit) & 0x01;
                out_symbols[sym_idx].i = val ? 1.0 : -1.0;
                out_symbols[sym_idx].q = 0.0;
                sym_idx++;
            }
        }
    } else if (mod == MOD_QPSK) {
        for (size_t b = 0; b < num_bytes; b++) {
            for (int shift = 6; shift >= 0; shift -= 2) {
                uint8_t dibit = (bytes[b] >> shift) & 0x03;
                /* Mapeo Gray normalizado (Es = 1) */
                uint8_t bit_i = (dibit >> 1) & 0x01;
                uint8_t bit_q = dibit & 0x01;
                out_symbols[sym_idx].i = (bit_i ? 1.0 : -1.0) * INV_SQRT2;
                out_symbols[sym_idx].q = (bit_q ? 1.0 : -1.0) * INV_SQRT2;
                sym_idx++;
            }
        }
    }
    return sym_idx;
}

size_t demodulate_symbols(const IQSample *symbols, size_t num_symbols, ModulationType mod, uint8_t *out_bytes) {
    size_t byte_idx = 0;

    if (mod == MOD_BPSK) {
        for (size_t s = 0; s < num_symbols; s += 8) {
            uint8_t current_byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                uint8_t decision = (symbols[s + bit].i >= 0.0) ? 1 : 0;
                current_byte = (current_byte << 1) | decision;
            }
            out_bytes[byte_idx++] = current_byte;
        }
    } else if (mod == MOD_QPSK) {
        for (size_t s = 0; s < num_symbols; s += 4) {
            uint8_t current_byte = 0;
            for (int pair = 0; pair < 4; pair++) {
                uint8_t bit_i = (symbols[s + pair].i >= 0.0) ? 1 : 0;
                uint8_t bit_q = (symbols[s + pair].q >= 0.0) ? 1 : 0;
                uint8_t dibit = (bit_i << 1) | bit_q;
                current_byte = (current_byte << 2) | dibit;
            }
            out_bytes[byte_idx++] = current_byte;
        }
    }
    return byte_idx;
}