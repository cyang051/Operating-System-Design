/* idt.h - Defines various idt construction functions and 
 * handler functions managing exeptions, interrupts, and system calls
 */

#ifndef _IDT_H
#define _IDT_H

#include "types.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "drivers/terminal.h" // process keyboard interrupts
#include "drivers/rtc.h"
#include "syshandler.h"

void EXCEP0(); 	// 0x00	Division by zero	
void EXCEP1();	// 0x01	Debugger	
void EXCEP2();	// 0x02	NMI	
void EXCEP3();	// 0x03	Breakpoint	
void EXCEP4();	// 0x04	Overflow	
void EXCEP5();	// 0x05	Bounds	
void EXCEP6();	// 0x06	Invalid Opcode	
void EXCEP7();	// 0x07	Coprocessor not available	
void EXCEP8();	// 0x08	Double fault	
void EXCEP9();	// 0x09	Coprocessor Segment Overrun (386 or earlier only)	
void EXCEPA();	// 0x0A	Invalid Task State Segment	
void EXCEPB();	// 0x0B	Segment not present	
void EXCEPC();	// 0x0C	Stack Fault	
void EXCEPD();	// 0x0D	General protection fault	
void EXCEPE();	// 0x0E	Page fault	
void EXCEPF();	// 0x0F	reserved	
void EXCEP10();	// 0x10	Math Fault	
void EXCEP11();	// 0x11	Alignment Check	
void EXCEP12();	// 0x12	Machine Check	
void EXCEP13();	// 0x13	SIMD Floating-Point Exception	
void EXCEP14();	// 0x14	Virtualization Exception	
void EXCEP15();	// 0x15	Control Protection Exception
/* loop indefinitely after exception message */
void excep_loop();

void SYSTEMCALL(); 			// 0x80 System Call Handler
void KEYBOARD_HANDLER();   	// 0x21 Keyboard Hardware Interrupt Handler
void RTC_HANDLER();  		// 0x28 Real Time Clock Interrupt Handler

/* initializes idt */
extern void init_idt();
/* sets all idt values to point to the ignore exception (null exception) */
void setup_idt();
/* sets values 0 - 21 in the IDT to handle various exceptions */
void idt_exceptions();
/* creates and places interrupt descriptor in IDT based of parameters */
void set_interrupt(void (*HANDLER), int i);
/* creates and places trap descriptor in IDT based of parameters */
void set_trap(void (*HANDLER), int i);

int CP2_RTC_FLAG;


#endif /* _IDT_H */
