/* idt.c -  Defines various handler functions managing exeptions, 
 * interrupts, and system calls
 */

#include "idt.h"

#define KEYBOARD_PORT 0x60
#define SYS_IDT 0x80
#define KEYBOARD_IDT 0x21
#define RTC_IDT 0x28
#define NUM_VEC 256
#define FIRST16 0xFFFF0000
#define LAST16 0x0000FFFF
#define KEYB_IRQ 1
#define RTC_IRQ 8

volatile int rtc_interrupt_ocurred;

/* Exception handlers: - same for EXCEP0 - EXCEP15
	   Input: None
	Function: Handle cooresponding exception
		      (currently just prints and loops indefinitely)
*/
void EXCEP0()
{
	printf("Division by zero\n");
	excep_loop();
}
void EXCEP1()
{
	printf("Debugger\n");
	excep_loop();
}
void EXCEP2()
{
	printf("NMI\n");
	excep_loop();
}
void EXCEP3()
{
	printf("Breakpoint\n");
	excep_loop();
}
void EXCEP4()
{
	printf("Overflow\n");
	excep_loop();
}
void EXCEP5()
{
	printf("Bounds\n");
	excep_loop();
}
void EXCEP6()
{
	printf("Invalid Opcode\n");
	excep_loop();
}
void EXCEP7()
{
	printf("Coprocessor not available\n");
	excep_loop();
}
void EXCEP8()
{
	printf("Double fault\n");
	excep_loop();
}
void EXCEP9()
{
	printf("Coprocessor Segment Overrun (386 or earlier only)\n");
	excep_loop();
}
void EXCEPA()
{
	printf("Invalid Task State Segment\n");
	excep_loop();
}
void EXCEPB()
{
	printf("Segment not present\n");
	excep_loop();
}
void EXCEPC()
{
	printf("Stack Fault\n");
	excep_loop();
}
void EXCEPD()
{
	printf("General protection fault\n");
	excep_loop();
}
void EXCEPE()
{
	uint32_t pfa;
	asm("movl %%cr2, %%eax;"
		: "=a"(pfa)
		: // no input
		: "cc"
		);
	printf("Page fault at %d\n", pfa);
	excep_loop();
}
void EXCEPF()
{
	printf("reserved\n");
	excep_loop();
}
void EXCEP10()
{
	printf("Math Fault\n");
	excep_loop();
}
void EXCEP11()
{
	printf("Alignment Check\n");
	excep_loop();
}
void EXCEP12()
{
	printf("Machine Check\n");
	excep_loop();
}
void EXCEP13()
{
	printf("SIMD Floating-Point Exception\n");
	excep_loop();
}
void EXCEP14()
{
	printf("Virtualization Exception\n");
	excep_loop();
}
void EXCEP15()
{
	printf("Control Protection Exception\n");
	excep_loop();
}

void EXCEPIGNORE()
{
	printf("Unknown Interrupt\n");
	excep_loop();
}

/* excep_loop
	   Input: none
	Function: mask interrupts and loop indefinitely
 */
void excep_loop()
{
	cli();
	while(1);
}

/* KEYBOARD_HANDLER
	   Input: none
	Function: Handles keyboard interrupts
 */
void KEYBOARD_HANDLER()
{
	asm volatile("pushal;");
	cli();

	unsigned char scan_code = inb(KEYBOARD_PORT); // 0x60 = keyboard signal port
	process_key(scan_code);

	sti();
	send_eoi(KEYB_IRQ);
	asm volatile("popal; leave;	iret;");
}

/* RTC_HANDLER
	   Input: none
	Function: Handles rtc interrupts
 */
void RTC_HANDLER()
{
	asm volatile("pushal;");
	cli();

	process_rtc();

	sti();
	send_eoi(RTC_IRQ);
	asm volatile("popal; leave; iret;");
}

/* init_idt
    	INPUT: none
 	 FUNCTION: builds idt by calling local functions
 */
void init_idt() {
	setup_idt();
	idt_exceptions();
	set_trap(syscallhandle, (int)SYS_IDT); // 0x80 = location in idt for systemcall
	set_interrupt(KEYBOARD_HANDLER, (int)KEYBOARD_IDT); // 0x21 = location in idt for IRQ1
	set_interrupt(RTC_HANDLER, (int)RTC_IDT); // 0x28 = location in idt for IRQ8

	open_rtc(NULL);

	enable_irq(KEYB_IRQ);
	enable_irq(RTC_IRQ);
}

/* setup_idt
    	INPUT: none
 	 FUNCTION: set all values of IDT to the EXCEPIGNORE exception (null exception) 
 */
void setup_idt() {
	int i;
	for(i = 0; i <NUM_VEC; i++)
		set_interrupt(EXCEPIGNORE, i);
}

/* idt_exceptions
    	INPUT: none
 	 FUNCTION: set all values of IDT to correctly handle exceptions 
 */
void idt_exceptions()	{ 
	int i;
	void (*EXCEPTHANDLRPTR[22])() = { // 22 = number of exceptions
		&EXCEP0, &EXCEP1, &EXCEP2, &EXCEP3, &EXCEP4, 
		&EXCEP5, &EXCEP6, &EXCEP7, &EXCEP8, &EXCEP9,
		&EXCEPA, &EXCEPB, &EXCEPC, &EXCEPD, &EXCEPE, 
		&EXCEPF, &EXCEP10, &EXCEP11, &EXCEP12, &EXCEP13, 
		&EXCEP14, &EXCEP15};
	for(i = 0; i <22; i++) {
		set_interrupt(EXCEPTHANDLRPTR[i], i);
	}
}

/*  set_interrupt
		INPUT: HANDLER - address of handler function for cooresponding descriptor
			   i - location in IDT to place descriptor
     FUNCTION: creates and place descriptor in IDT based of parameters
*/
void set_interrupt(void (*HANDLER), int i) {
	idt[i].seg_selector = KERNEL_CS;
	idt[i].reserved4 = 	0x00;
	idt[i].reserved3 =	0;
	idt[i].reserved2 =	1;
	idt[i].reserved1 =	1;
	idt[i].size =		1;
	idt[i].reserved0 =	0;
	idt[i].dpl =	 	0; // 0 = unprivileged access
	idt[i].present = 	1;
	idt[i].offset_31_16 =  ((int)HANDLER & FIRST16) >> 16; // mask and bit shift highest 2 bytes
	idt[i].offset_15_00 = (int)HANDLER & LAST16;  // mask lowest 2 bytes
}

/*  set_trap
		INPUT: HANDLER - address of handler function for cooresponding descriptor
			   i - location in IDT to place descriptor
     FUNCTION: creates and place descriptor in IDT based of parameters in trap mode
*/
void set_trap(void (*HANDLER), int i) {
	idt[i].seg_selector = KERNEL_CS;
	idt[i].reserved4 = 	0x00;
	idt[i].reserved3 =	1;
	idt[i].reserved2 =	1;
	idt[i].reserved1 =	1;
	idt[i].size =		1;
	idt[i].reserved0 =	0;
	idt[i].dpl =	 	3; // 3 = privileged access
	idt[i].present = 	1;
	idt[i].offset_31_16 =  ((int)HANDLER & FIRST16) >> 16; // mask and bit shift highest 2 bytes
	idt[i].offset_15_00 = (int)HANDLER & LAST16;  // mask lowest 2 bytes
}

