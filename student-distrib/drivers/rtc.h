/* rtc.h - Defines rtc driver
 */

#ifndef _RTC_H
#define _RTC_H

#include "../types.h"
#include "../lib.h"
#include "../idt.h"

/* handler for ticks */
void process_rtc();
/* open */
int32_t open_rtc(const uint8_t* filename);
/* read */
int32_t read_rtc(int32_t fd, void* buf, int32_t nbytes);
/* write */
int32_t write_rtc(int32_t fd, const void* buf, int32_t nbytes);
/* close */
int32_t close_rtc(int32_t fd);

volatile int rtc_interrupt_occurred;

#endif /* _RTC_H */
