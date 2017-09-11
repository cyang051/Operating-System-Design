/* terminal.h - Defines terminal keyboard driver
 */

#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "../types.h"
#include "../x86_desc.h"
#include "../lib.h"
#include "../i8259.h"
#include "../idt.h"
#include "../syscalls.h"
#include "rtc.h"

/* holds the current keyboard buffer */
unsigned char term_buffer[3][128];
/* holds the current keyboard buffer location to add a character */
unsigned short term_loc[3];
// unsigned short read_loc;
char video_buffer[3][4000];

/* used to toggle corresponding key */
unsigned short ctrl_l;
unsigned short alt_l;
unsigned short shift;
unsigned short capslock;
/* flag to check if entered was recently pressed */
unsigned short entered;
/* current terminal : 0, 1, or 2 */
int current_terminal;

/* initializes variables and clears screen */
void init_terminal(void);
/* processes scan_code of currently pressed key */
void process_key(unsigned char scan_code);
/* moves current keyboard buffer to the top */
void buff_to_top(void);

/* sys_read handlers for stdout and stdin */
int32_t stdout_read (int32_t fd, void* buf, int32_t nbytes);
int32_t stdin_read (int32_t fd, void* buf, int32_t nbytes);
/* sys_write handlers for stdout and stdin */
int32_t stdout_write (int32_t fd, const void* buf, int32_t nbytes);
int32_t stdin_write (int32_t fd, const void* buf, int32_t nbytes);
/* sys_open handlers for terminal (same for stdout/stdin) */
int32_t terminal_open (const uint8_t* filename);
/* sys_close handlers for terminal (same for stdout/stdin) */
int32_t terminal_close (int32_t fd);

#endif /* _TERMINAl_H */
