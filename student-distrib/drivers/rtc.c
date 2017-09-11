#include "rtc.h"

#define RTC_PORT1 0x70
#define RTC_PORT2 0x71
#define REGC 0x0C
#define REGA 0x8A

#define LOW_4_MASK 0xF0
#define SELECT_REGB 0x8B
#define SELECTBIT6 0x40
#define FREQ_2 15
#define FREQ_4 14
#define FREQ_8 13
#define FREQ_16 12
#define FREQ_32 11
#define FREQ_64 10
#define FREQ_128 9
#define FREQ_256 8
#define FREQ_512 7
#define FREQ_1024 6


/* process_rtc
 *    INPUT: none
 * FUNCTION: process the rtc
 */
void process_rtc()
{
	outb(REGC,RTC_PORT1);	// select register C
	inb(RTC_PORT2);		// just throw away contents
	rtc_interrupt_occurred = 1;	// interrupt occured for rtc_read()
	if(CP2_RTC_FLAG)
		printf("1");
}

/* open_rtc
 *    INPUT: see sys_open in syscall.c
 * FUNCTION: opens RTC device
 */
int32_t open_rtc(const uint8_t* filename)
{
	cli();			// disable interrupts
	outb(SELECT_REGB,RTC_PORT1);		// select register B, and disable NMI
	char prev = inb(RTC_PORT2);	// read the current value of register B
	outb(SELECT_REGB,RTC_PORT1);		// set the index again (a read will reset the index to register D)
	outb(prev | SELECTBIT6,RTC_PORT2);	// write the previous value ORed with 0x40. This turns on bit 6 of register B
	sti();

	int frequency = 2;
	write_rtc(0, &frequency, 4); // initialize to 2 Hz

	return 0;
}

/* read_rtc
 *    INPUT: see sys_read in syscalls.c
 * FUNCTION: returns when rtc tick/interrupt occurred
 */
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes)
{
	cli();
	rtc_interrupt_occurred = 0;
	sti();
	while(!rtc_interrupt_occurred);
	return 0;
}

/* write_rtc
 *    INPUT: see sys_write in syscalls.c
 * FUNCTION: writes coor. frequency to rtc if power of 2 (up to 1024)
 */
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes)
{
	int rate;
	switch(*(int*)buf) {
		case(2):
			rate = FREQ_2;
			break;
		case(4):
			rate = FREQ_4;
			break;
		case(8):
			rate = FREQ_8;
			break;
		case(16):
			rate = FREQ_16;
			break;
		case(32):
			rate = FREQ_32;
			break;
		case(64):
			rate = FREQ_64;
			break;
		case(128):
			rate = FREQ_128;
			break;
		case(256):
			rate = FREQ_256;
			break;
		case(512):
			rate = FREQ_512;
			break;
		case(1024):
			rate = FREQ_1024;
			break;
		default:
			return -1;
	}
	cli();
	outb(REGA, RTC_PORT1);	// disable non-maskable interrupts
	unsigned char prev = inb(RTC_PORT2);	// get current value of register A
	outb(REGA, RTC_PORT1);	// set index to regsiter A
	outb((prev & LOW_4_MASK) | rate, RTC_PORT2);	// modify rate of RTC
	sti();
	return 0;
}

/* close_rtc
 *    INPUT: see sys_close in syscalls.c
 * FUNCTION: blank handler for close rtc
 */
int32_t close_rtc(int32_t fd)
{
	return -1;
}
