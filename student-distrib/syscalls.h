#ifndef _syscalls_H
#define _syscalls_H

#include "x86_desc.h"
#include "filesystem.h"
#include "paging.h"
#include "lib.h"
#include "drivers/terminal.h"
#include "drivers/rtc.h"
#define TOTAL_NUMBER_OF_FILE_DESCRIPTORS 8
#define ARG_SIZE 1024
#define FLAG_UNSET 0



/*  
 syscalls 0 - 10
 */ 
 int32_t sys_zero();
 int32_t sys_halt (uint8_t status);
 int32_t sys_execute (const uint8_t* command);
 int32_t sys_read (int32_t fd, void* buf, int32_t nbytes);
 int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes);
 int32_t sys_open (const uint8_t* filename);
 int32_t sys_close (int32_t fd);
 int32_t sys_getargs (uint8_t* buf, int32_t nbytes);
 int32_t sys_vidmap (uint8_t** screen_start);
 int32_t sys_set_handler (int32_t signum, void* handler);
 int32_t sys_sigreturn (void);


/*
file_d: (file descriptor)
1. file_d_jump : jumptable for open,read,write,close
2. inode_num : 0 for dir and RTC dev file
3. file_pos : file position. read should update this
4. flag : flag to mark as in-use
*/
typedef struct __attribute__((packed)) file_descrip {
	uint32_t *file_d_jump;
	uint32_t inode_num;
	uint32_t file_pos;
	uint32_t flag; // 1 for in use, 0 for not in use
} file_d;

void process_start_file_d(file_d *process_fds);


/*
PCB:
1. esp : stack pointer for process
2. ebp : base pointer for process
3. parent : ptr to parent process
4. child : ptr to child process
5. open_files : // upto 8 open files, holds file descriptor of file
*/
typedef struct __attribute__((packed)) PCB_struct {
	uint32_t esp;
	uint32_t ebp;
	uint32_t p_id;
	uint32_t parent;
	uint32_t child;
	file_d fd[TOTAL_NUMBER_OF_FILE_DESCRIPTORS];
	uint8_t args[ARG_SIZE];
	uint32_t size_args;
} PCB;

int process_array[6];
PCB control_block[6];


// Helper Functions
PCB* get_pcb_ptr();
file_d* get_available_fd(uint32_t* return_file_descriptor_index);
#endif
