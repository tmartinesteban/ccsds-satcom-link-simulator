import csv
import numpy as np
import matplotlib.pyplot as plt
from scipy.special import erfc

def load_iq_symbols(csv_path):
    i_vals, q_vals = [], []
    with open(csv_path, mode="r") as f:
        reader = csv.DictReader(f)
        for row in reader:
            i_vals.append(float(row["I"]))
            q_vals.append(float(row["Q"]))
    return np.array(i_vals) + 1j * np.array(q_vals)

def compute_leo_link_budget(freq_hz=2.2e9, slant_range_km=1200.0, tx_power_dbw=3.0,
                            tx_gain_dbi=6.0, rx_gain_dbi=24.0, sys_temp_k=290.0,
                            bit_rate_bps=1e6):
    """Calcula el presupuesto de enlace descendente (Downlink) para un satélite LEO en Banda S."""
    c = 299792458.0
    k_boltzmann = -228.6  # dBW / (K * Hz)
    wavelength = c / freq_hz
    distance_m = slant_range_km * 1e3

    # Pérdidas en espacio libre (FSPL)
    fspl_db = 20 * np.log10(4 * np.pi * distance_m / wavelength)
    misc_losses_db = 2.5  # Apuntamiento, polarización y atmósfera

    eirp_dbw = tx_power_dbw + tx_gain_dbi
    rx_power_dbw = eirp_dbw - fspl_db - misc_losses_db + rx_gain_dbi
    n0_dbw_hz = k_boltzmann + 10 * np.log10(sys_temp_k)
    cn0_dbhz = rx_power_dbw - n0_dbw_hz
    eb_n0_db = cn0_dbhz - 10 * np.log10(bit_rate_bps)

    return {
        "FSPL_dB": fspl_db,
        "EIRP_dBW": eirp_dbw,
        "Rx_Power_dBW": rx_power_dbw,
        "CN0_dBHz": cn0_dbhz,
        "Eb_N0_dB": eb_n0_db
    }

def add_awgn_and_doppler(symbols, eb_n0_db, residual_doppler_hz=150.0, symbol_rate=5e5):
    """Inyecta ruido AWGN calibrado por Eb/N0 y rotación de fase por Doppler residual."""
    bits_per_symbol = 2  # QPSK
    es_n0_db = eb_n0_db + 10 * np.log10(bits_per_symbol)
    es_n0_linear = 10 ** (es_n0_db / 10.0)

    # Desviación estándar del ruido complejo (potencia señal Es = 1)
    noise_sigma = np.sqrt(1.0 / (2.0 * es_n0_linear))
    rng = np.random.default_rng(seed=42)
    noise = noise_sigma * (rng.standard_normal(len(symbols)) + 1j * rng.standard_normal(len(symbols)))

    # Desviación de fase residual tras bucle PLL (Doppler no compensado)
    t = np.arange(len(symbols)) / symbol_rate
    phase_drift = np.exp(1j * 2 * np.pi * residual_doppler_hz * t)

    return symbols * phase_drift + noise

def demodulate_qpsk_bits(rx_symbols):
    """Demodulador Hard-Decision QPSK (mapeo Gray coherente con C)."""
    bit_i = (np.real(rx_symbols) >= 0.0).astype(np.uint8)
    bit_q = (np.imag(rx_symbols) >= 0.0).astype(np.uint8)
    bits = np.empty(len(rx_symbols) * 2, dtype=np.uint8)
    bits[0::2] = bit_i
    bits[1::2] = bit_q
    return bits

def main():
    tx_symbols = load_iq_symbols("build/tx_iq.csv")
    tx_bits = demodulate_qpsk_bits(tx_symbols)  # Bits originales limpios

    # 1. Calcular Link Budget LEO real
    budget = compute_leo_link_budget()
    print("=== LEO S-BAND DOWNLINK BUDGET (2.2 GHz, 1200 km slant range) ===")
    for k, v in budget.items():
        print(f"  {k:<14}: {v:.2f}")

    # 2. Barrido de Eb/N0 para curva BER (Teórica vs Simulada)
    eb_n0_range = np.arange(0, 11, 1)
    ber_sim = []
    ber_theo = 0.5 * erfc(np.sqrt(10 ** (eb_n0_range / 10.0)))

    for eb in eb_n0_range:
        rx_sym = add_awgn_and_doppler(tx_symbols, eb_n0_db=eb, residual_doppler_hz=0.0)
        rx_bits = demodulate_qpsk_bits(rx_sym)
        errors = np.sum(tx_bits != rx_bits)
        ber_sim.append(max(errors / len(tx_bits), 1e-6))

    # 3. Generar gráficas para documentación (docs/)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    # Subgráfica 1: Constelación QPSK recibida con Eb/N0 = 7 dB y Doppler residual
    rx_demo = add_awgn_and_doppler(tx_symbols[:2000], eb_n0_db=7.0, residual_doppler_hz=45.0)
    ax1.scatter(np.real(rx_demo), np.imag(rx_demo), s=6, alpha=0.5, color="#1f77b4", label="RX Symbols (7 dB)")
    ideal = np.array([1+1j, 1-1j, -1+1j, -1-1j]) / np.sqrt(2)
    ax1.scatter(np.real(ideal), np.imag(ideal), s=90, color="#d62728", marker="x", linewidths=2, label="Ideal QPSK")
    ax1.axhline(0, color="gray", lw=0.8, ls="--")
    ax1.axvline(0, color="gray", lw=0.8, ls="--")
    ax1.set_title("QPSK Constellation (Eb/N0 = 7 dB + Residual Doppler)")
    ax1.set_xlabel("In-Phase (I)")
    ax1.set_ylabel("Quadrature (Q)")
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Subgráfica 2: Curva BER QPSK
    ax2.semilogy(eb_n0_range, ber_theo, "k--", lw=1.8, label="Theoretical QPSK BER")
    ax2.semilogy(eb_n0_range, ber_sim, "o-", color="#2ca02c", lw=1.8, label="Simulated CCSDS Link")
    ax2.set_title("Bit Error Rate (BER) Performance over AWGN")
    ax2.set_xlabel("Eb/N0 (dB)")
    ax2.set_ylabel("Bit Error Rate (BER)")
    ax2.set_ylim(1e-5, 1)
    ax2.grid(True, which="both", alpha=0.3)
    ax2.legend()

    plt.tight_layout()
    plt.savefig("docs/link_performance.png", dpi=200)
    print("\nOK: Gráfica guardada en docs/link_performance.png")

if __name__ == "__main__":
    main()