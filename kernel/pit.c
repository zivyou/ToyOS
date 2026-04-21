#include "pit.h"
#include "common.h"
#include "printk.h"

/**
 * pit_init - Initialize the PIT (Programmable Interval Timer)
 *
 * Configures channel 0 to generate periodic interrupts at TIMER_FREQ_HZ.
 * Channel 0 is connected to IRQ 0, which is mapped to interrupt 32 in the IDT.
 */
/**
 * Intel 8254芯片,会以100Hz的频率触发定频脉冲信号;
 * i386架构下, 8254芯片会连接上8259A中断控制器芯片, 8254产生的信号会转换成8259A的32号irq
 */
void pit_init(void) {
    uint16_t divisor = TIMER_DIVISOR;

    // Send command to PIT
    // Channel 0, access low/high byte, mode 3 (square wave), binary mode
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_WORD | PIT_CMD_MODE3 | PIT_CMD_BINARY);

    // Send divisor (low byte then high byte)
    outb(PIT_CHANNEL0, divisor & 0xFF);        // Low byte
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF); // High byte

    printk("pit_init: PIT configured at %d Hz (divisor: %d)\n",
           TIMER_FREQ_HZ, divisor);
}
