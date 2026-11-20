CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lm

SRC_DIR = src/c_core
BUILD_DIR = build
TARGET = $(BUILD_DIR)/satcom_tx

SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/ccsds_frame.c $(SRC_DIR)/modulator.c

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

sim: run
	python3 src/python_sim/channel_sim.py

clean:
	rm -rf $(BUILD_DIR)/*

.PHONY: all run sim clean