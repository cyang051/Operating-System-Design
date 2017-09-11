/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts
 * are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7 */
uint8_t slave_mask; /* IRQs 8-15 */

#define BITMASK 0xFF
 #define SLAVE_IRQ 2
 #define MASTER_IRQS 8
 #define OUT_OF_SLAVE_IRQ_BOUND 15
/* Initialize the 8259 PIC */
void
i8259_init(void)
{
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_8259_PORT + 1);
	outb(ICW3_MASTER, MASTER_8259_PORT + 1);
	outb(ICW4, MASTER_8259_PORT + 1);

	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);
	outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);
	outb(ICW4, SLAVE_8259_PORT + 1);

	outb(BITMASK, MASTER_8259_PORT+1);
	outb(BITMASK, SLAVE_8259_PORT+1);
    enable_irq(SLAVE_IRQ);

}

/* Enable (unmask) the specified IRQ */
void
enable_irq(uint32_t irq_num)
{
	uint16_t port;
   	uint8_t value;
 	if(irq_num >OUT_OF_SLAVE_IRQ_BOUND || irq_num <0)
 		return;
    if(irq_num < MASTER_IRQS) {
        port = MASTER_8259_PORT + 1;
    } else {
        port = SLAVE_8259_PORT + 1;
        irq_num -= 8;
    }
    value = ~(inb(port));
    value=  value | (1 << irq_num);
    value= ~value;
    outb(value, port);        
}

/* Disable (mask) the specified IRQ */
void
disable_irq(uint32_t irq_num)
{
	uint16_t port;
    uint8_t value;
 	if(irq_num >OUT_OF_SLAVE_IRQ_BOUND || irq_num <0)
 		return;
    if(irq_num < MASTER_IRQS) {
        port = MASTER_8259_PORT + 1;
    } else {
        port = SLAVE_8259_PORT + 1;
        irq_num -= 8;
    }
    value = ~(inb(port));
    value = value & ~(1 << irq_num);
    value= ~value;
    outb(value, port);  
}

/* Send end-of-interrupt signal for the specified IRQ */
extern void
send_eoi(uint32_t irq_num)
{
	if(irq_num >= MASTER_IRQS){
        outb(EOI | (irq_num-8), SLAVE_8259_PORT);
        outb(EOI + 2, MASTER_8259_PORT);
 	} else {
	    outb(EOI | irq_num, MASTER_8259_PORT);
    }
}

