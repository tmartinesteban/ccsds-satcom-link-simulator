#ifndef MODULATOR_H
#define MODULATOR_H

#include <stdint.h>
#include <stddef.h>

typedef enum {
    MOD_BPSK = 1,
    MOD_QPSK = 2
} ModulationType;

typedef struct {
    double i; /* Componente en fase (In-Phase) */
    double q; /* Componente en cuadratura (Quadrature) */
} IQSample;

/* Modula un buffer de bytes a símbolos complejos normalizados (energía media Es = 1) */
size_t modulate_bytes(const uint8_t *bytes, size_t num_bytes, ModulationType mod, IQSample *out_symbols);

/* Demodula símbolos I/Q ruidosos (Hard-Decision) de vuelta a bytes */
size_t demodulate_symbols(const IQSample *symbols, size_t num_symbols, ModulationType mod, uint8_t *out_bytes);

#endif /* MODULATOR_H */