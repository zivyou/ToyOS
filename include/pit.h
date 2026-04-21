#ifndef PIT_H
#define PIT_H

#include "types.h"

// PIT I/O Ports
#define PIT_CHANNEL0    0x40    // Channel 0 data port (read/write)
#define PIT_CHANNEL1    0x41    // Channel 1 data port (read/write)
#define PIT_CHANNEL2    0x42    // Channel 2 data port (read/write)
#define PIT_COMMAND     0x43    // Command/Mode register

// PIT frequency (Hz) - standard PC timer frequency
#define PIT_FREQUENCY   1193182

// Timer interrupt frequency (Hz)
// 100 Hz = 10ms per tick
#define TIMER_FREQ_HZ   100

// Calculate divisor for desired frequency
// divisor = PIT_FREQUENCY / desired_frequency
#define TIMER_DIVISOR   (PIT_FREQUENCY / TIMER_FREQ_HZ)

// PIT command bits
#define PIT_CMD_CHANNEL0    (0x0 << 6)  // Select channel 0
#define PIT_CMD_CHANNEL1    (0x1 << 6)  // Select channel 1
#define PIT_CMD_CHANNEL2    (0x2 << 6)  // Select channel 2
#define PIT_CMD_READBACK    (0x3 << 6)  // Read-back command

#define PIT_CMD_LATCH       (0x0 << 4)  // Latch count value command
#define PIT_CMD_LOBYTE      (0x1 << 4)  // Access low byte only
#define PIT_CMD_HIBYTE      (0x2 << 4)  // Access high byte only
#define PIT_CMD_WORD        (0x3 << 4)  // Access low then high byte

#define PIT_CMD_MODE0       (0x0 << 1)  // Interrupt on terminal count
#define PIT_CMD_MODE1       (0x1 << 1)  // Hardware re-triggerable one-shot
#define PIT_CMD_MODE2       (0x2 << 1)  // Rate generator
#define PIT_CMD_MODE3       (0x3 << 1)  // Square wave generator
#define PIT_CMD_MODE4       (0x4 << 1)  // Software triggered strobe
#define PIT_CMD_MODE5       (0x5 << 1)  // Hardware triggered strobe

#define PIT_CMD_BINARY      (0x0 << 0)  // Binary counting mode
#define PIT_CMD_BCD         (0x1 << 0)  // BCD counting mode

/**
 * pit_init - Initialize the PIT (Programmable Interval Timer)
 *
 * Configures channel 0 to generate periodic interrupts at TIMER_FREQ_HZ
 */
void pit_init(void);

#endif // PIT_H
