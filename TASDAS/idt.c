#include "idt.h"



void 
idt_init() 
{
	/*
	typedef union idt_desc_t {
		uint32_t val;
		struct {
			uint16_t offset_15_00;
			uint16_t seg_selector;
			uint8_t reserved4;
			uint32_t reserved3 : 1;
			uint32_t reserved2 : 1;
			uint32_t reserved1 : 1;
			uint32_t size : 1;
			uint32_t reserved0 : 1;
			uint32_t dpl : 2;
			uint32_t present : 1;
			uint16_t offset_31_16;
		} __attribute__((packed));
	} idt_desc_t;
	*/
	SET_IDT_ENTRY(idt[0], printf("Hello\n"));
	idt[0].seg_selector = KERNEL_CS;
	idt[0].reserved4 = 0x0;
	idt[0].reserved3 = 0;
	idt[0].reserved2 = 1;
	idt[0].reserved1 = 1;
	idt[0].reserved0 = 0;
	idt[0].size = 1; //1 for 32 bits gate, 0 for 16 bits gate
	idt[0].dpl = 0;
	idt[0].present = 1;
}
