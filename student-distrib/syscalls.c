#include "syscalls.h"
#include "filesystem.h"
// address constants
#define _8KB 0x2000
#define _4MB 0x400000
#define _8MB 0x800000
#define _128MB  0x8000000
#define _132MB  0x8400000
#define IMAGE_OFFSET 0x48000
#define PROGRAM_IMG_START _128MB + IMAGE_OFFSET

// various constants
#define FILE_SIZE 32
#define FIRST_NON_STD_FD 2
#define FLAG_SET 1
#define FLAG_UNSET 0
#define STDIN 0
#define STDOUT 1
#define FILE_START 0
#define ERROR -1
#define MAX_FD_INDEX 7
#define MIN_FD_INDEX 0

#define INITIAL_PID 0
#define FIRST_PROCESS_PID 0
#define MAX_NUM_PROCESSES 6

#define NUM_FILE_OPS 4
#define FILE_OP_OPEN 0
#define FILE_OP_READ 1
#define FILE_OP_WRITE 2
#define FILE_OP_CLOSE 3

/* jump tables for coor. ORWC handlers */
uint32_t rtc_jump[NUM_FILE_OPS] = {
	(uint32_t)open_rtc, (uint32_t)read_rtc, (uint32_t)write_rtc, (uint32_t)close_rtc
};
uint32_t stdin_jump[NUM_FILE_OPS] = {
	(uint32_t)terminal_open, (uint32_t)stdin_read, (uint32_t)stdin_write, (uint32_t)terminal_close
};
uint32_t stdout_jump[NUM_FILE_OPS] = {
	(uint32_t)terminal_open, (uint32_t)stdout_read, (uint32_t)stdout_write, (uint32_t)terminal_close
};
uint32_t file_jump[NUM_FILE_OPS] = {
	(uint32_t)open_file, (uint32_t)read_data_corr_sig, (uint32_t)file_write,(uint32_t)file_close
};
uint32_t directory_jump[NUM_FILE_OPS]=	{
	(uint32_t)open_directory, (uint32_t)read_directory, (uint32_t)directory_write, (uint32_t)directory_close
};

PCB * pc_block;

/* sys_execute
	INPUT:
		command - file name of program being executed
	FUNCTION:
		Executes corresponding file as a process
		Returns -
			-1    : command can't be executed
			256   : program dies by exeption
			0-255 : program executes `halt` system call
*/
int32_t sys_execute (const uint8_t* command)
{
	cli();
	int i = 0, arg_i = 0, current_process = INITIAL_PID, child_process;
	int entry_point;
	uint8_t file[FILE_SIZE]; // 32 = max file name length in file_system
	uint8_t args[ARG_SIZE]; // 1024 = max size of buffer
	uint8_t buffer[4];


	//---------- PARSE ARGS ----------//
	for(i=0; i < FILE_SIZE && command[i] != '\0' && command[i] != ' '; i++)
		file[i] = command[i];
	if(i < FILE_SIZE)
		file[i] = '\0';

	while(i < ARG_SIZE && command[i] != '\0'){
		// skip leading spaces
		for(i=i; command[i] == ' ' && i < ARG_SIZE; i++);
		// copy arg into args
		for(i=i; command[i] != '\0' && command[i] != ' '; i++, arg_i++)
			args[arg_i] = command[i];
		if(i < ARG_SIZE && command[i] != '\0'){
			args[arg_i] = ' ';
			arg_i++;
		}
	}

	/*---------- CHECK FILE VALIDITY ----------*/
	dentry_t file_dentry;
	if(read_dentry_by_name(file, &file_dentry) == ERROR)
		return ERROR; // command can't be executed 

	/*----------- CHECK IF PROCESS AVAIABLE ------------*/
	for(i = 0; i < MAX_NUM_PROCESSES; i++) {
		if(process_array[i] == 0) {
			current_process = i;
			process_array[i] = 1;
			break;
		}
		else if(i == 5) { // no process available (limit at 2 for cp3 [0,1]) (limit at 6)
			printf("Maximum number of active programs reached (6)\n");
			return 0; // don't return error, because behaves as expected
		}
	}

	/*---------- SET UP PAGING ----------*/
		// page starts at 128MB (virtual memory)
		// physical memory starts at 8MB + (process # * 4MB)
		// process # starts at 0
		// FLUSH TLB when swapping page
	program_paging(_8MB + (current_process * _4MB)); // 8MB is physical address of first program


	/*---------- LOAD FILE INTO MEMORY ----------*/
		// check ELF magic constant (0x7F, 0x45, 0x4C, 0x46)
		// copy file contents to mem location
		// find first instr address
	// check if file is executable (ELF)
	read_data(file_dentry.inode_number, 0, buffer, 4);
	if(!(buffer[0] == 0x7F && buffer[1] == 0x45 &&
		 buffer[2] == 0x4C && buffer[3] == 0x46))
		return ERROR;	// ELF not correct

	// read bytes 24-27 to get entry point into program
	read_data(file_dentry.inode_number, 24, buffer, 4); // used for context switch later "fake IRET", goes to EIP
	entry_point = *((uint32_t*)buffer);

	inode_t * file_inode = (inode_t *)boot_block + 1 + file_dentry.inode_number;
	// copy starting at virtual address 0x08048000
	read_data(file_dentry.inode_number, 0, (uint8_t*)PROGRAM_IMG_START, file_inode->length_in_bytes); 
	// idk how large length should be


	/*--------- CREATE PCB/Open FDs ----------*/
		// unique to each process
		// fd[0] = stdin , keyboard input
		// fd[1] = stdout , terminal output
		// fd[2-7] = dynamically assigned stuff

	// 1. Set up PCB at bottom of kernel page - 8kB * (process + 1)
	pc_block = (PCB*)(_8MB - _8KB * (current_process + 1) );
	pc_block->p_id = current_process;
	for(i=0; i<arg_i; i++)
		pc_block->args[i] = args[i]; // set pcb args to the parsed args
	pc_block->size_args = arg_i;
	// printf("arg: %s| %d\n", pc_block->args, pc_block->size_args);
		// check for available process
	asm("movl %%ebp, %%eax;"
		"movl %%esp, %%ebx;"
		: "=a"(pc_block->ebp), "=b"(pc_block->esp)
		:
		: "memory"
		);

	// 2. Open FDs
	process_start_file_d(pc_block->fd);


	/*---------- PREPARE FOR AND CALL IRET ----------*/
	// https://web.archive.org/web/20160326062442/http://jamesmolloy.co.uk/tutorial_html/10.-User%20Mode.html
	tss.ss0 = 0x18;	// 0x18 is KERNEL_DS
	tss.esp0 = _8MB - _8KB * current_process-4 ; // top of kernel page
	int32_t return_value;
	asm volatile("movl $0x2B, %%eax;"	// 0x2B is USER_DS
				 "movw %%ax, %%ds;"
				 "movl $0x083FFFFC, %%eax;" // 0x08400000 - 4 (bottom of 4 MB page holding executable)
				 "pushl $0x2B;"
				 "pushl %%eax;"
				 "pushfl;"	// push flags
				 "popl %%eax;"	// Enable IF flag so interrupts reenabled when IRET executed
				 "orl $0x200, %%eax;"
				 "pushl %%eax;"
				 "pushl $0x23;"	// 0x23 is USER_CS
				 "pushl %%ebx;"	// ebx contains entry_point
				 "iret;"
				 "halt_goto_label:"
				 : "=a"(return_value)
				 : "b"(entry_point)
				 : "memory", "cc"
				 );
	return return_value;
}

/* sys_halt
 *    INPUT: status (also the putput)
 * FUNCTION: halts current process, 
 *           if last shell, call execute again
 */
int32_t sys_halt (uint8_t status)
{
	PCB* pcb = get_pcb_ptr();
	int restart_flag = 0;
	if(pcb->p_id == FIRST_PROCESS_PID) {
	 	restart_flag = 1;
	}
	
	// restore to parent page
	program_paging(_8MB + ((pcb->p_id - 1) * _4MB));

	// restore to parent kernel stack
	tss.esp0 = _8MB - _8KB * (pcb->p_id - 1);

	// free up process
	process_array[pcb->p_id] = 0;

	if(restart_flag){
		sys_execute((uint8_t*)"shell");
	}

	// restore esp and ebp
	// avoid returning to caller
	asm("movl %%ebx, %%ebp;"
		"movl %%ecx, %%esp;"
		"jmp halt_goto_label;"
		: // no output
		: "a"((uint32_t)status), "b"(pcb->ebp), "c"(pcb->esp)
		);

	return 0;
}

/* sys_read
 *    INPUT: fd - file descriptor to read from
 *           buf - buffer to write read data to
 *           nbytes - size to read
 * FUNCTION: depends on fd_read
 */
int32_t sys_read (int32_t fd, void* buf, int32_t nbytes)
{
	//error handling
	if(fd < MIN_FD_INDEX || fd > MAX_FD_INDEX) // check for negative or very large fd's
		return ERROR;
	
	PCB* pcb = get_pcb_ptr();

	if (!pcb->fd[fd].flag)
	 	return ERROR;

	// call jump table
	uint32_t (*fd_read)(int32_t fd, void* buf, int32_t nbytes) = (void*)pcb->fd[fd].file_d_jump[FILE_OP_READ];
	return fd_read(fd, buf, nbytes);
}

/* sys_write
 *    INPUT: fd - file descriptor to write from
 *           buf - buffer to write from
 *           nbytes - size to write from buffer
 * FUNCTION: depends on fd_write
 */
int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes)
{
	// error handling
	if(fd < MIN_FD_INDEX || fd > MAX_FD_INDEX) // check for negative or large fd's
		return ERROR;

	PCB* pcb = get_pcb_ptr();

	if (!pcb->fd[fd].flag)
	 	return ERROR;

	// call jump table
	uint32_t (*fd_write)(int32_t fd, const void* buf, int32_t nbytes) = (void*)pcb->fd[fd].file_d_jump[FILE_OP_WRITE];
	return fd_write(fd, buf, nbytes);
}

/* sys_open
 *    INPUT: filename - file to open
 * FUNCTION: opens rtc,directory, or file
 *           returns - file descriptor index
 */
int32_t sys_open (const uint8_t* filename)
{
	dentry_t file_dentry;
	if(read_dentry_by_name(filename, &file_dentry) == ERROR)
		return ERROR; // file does not exist
	uint32_t fd_index = FILE_START;
	uint32_t* fd_index_ptr = &fd_index;
	file_d* fd = get_available_fd(fd_index_ptr);

	if(fd == NULL)
		return ERROR;

	switch(file_dentry.file_type) {
		case RTCFILETYPE :
			fd->file_d_jump = rtc_jump;
			fd->inode_num = FILE_START;
			fd->file_pos = 0;
			break;
		case DIRECTORYFILETYPE:
			fd->file_d_jump = directory_jump;
			fd->inode_num = FILE_START;
			fd->file_pos = 0;
			break;
		case REGULARFILETYPE:
			fd->file_d_jump = file_jump;
			fd->inode_num = file_dentry.inode_number;
			fd->file_pos = 0;
			break;
	}
	fd->file_pos = FILE_START;
	fd->flag = FLAG_SET;
	return fd_index;
}

/* sys_close
 *    INPUT: fd - file descriptor coor. to file closing
 * FUNCTION: closes file coor. to file descriptor
 			 return: depends on fd_close
 */
int32_t sys_close (int32_t fd)
{
	if(fd < FIRST_NON_STD_FD || fd > MAX_FD_INDEX) // exclude 0 and 1
		return ERROR;
	PCB* pcb = get_pcb_ptr();

	if (!pcb->fd[fd].flag)
	 	return ERROR;

	uint32_t (*fd_close)(int32_t fd);
	fd_close = (void*)pcb->fd[fd].file_d_jump[FILE_OP_CLOSE];

	pcb->fd[fd].flag = FLAG_UNSET;

	return (int32_t)fd_close;
}

/* sys_getargs
 *    INPUT: buf - buffer to store arguments from kernel into
 *			 nbytes - size of arguments buffer
 * FUNCTION: grab arguments of process from kernel, then move to
 *			 buf in user-space 
 */
int32_t sys_getargs (uint8_t* buf, int32_t nbytes)
{
	int i;
	PCB* pcb = get_pcb_ptr();

	if(nbytes < pcb->size_args - 1)  // -1 due to need for '\0'
		return ERROR;

	// 2. copy to `buf` in userspace
	for(i = 0; i < nbytes && i < pcb->size_args; i++){
		buf[i] = pcb->args[i];
	}
	buf[pcb->size_args] = '\0';
	// printf("arg2: %s| %d\n\n", buf, nbytes);

	return 0;
} 

/* sys_vidmap
 *    INPUT: screen_start -
 * FUNCTION:
 */
int32_t sys_vidmap (uint8_t** screen_start)
{
	// check that provided address is within user virtual address space
	if((uint32_t*)screen_start < (uint32_t*)_132MB && (uint32_t*)screen_start >= (uint32_t*)_128MB) {
		*screen_start = (uint8_t*)_132MB;
		return 0;
	}
	return ERROR;
}

/* sys_set_handler
 *    INPUT: signum -
 			 handler -
 * FUNCTION: empty handler, returns error (-1)
 */
int32_t sys_set_handler (int32_t signum, void* handler)
{
	return ERROR;
}

/* sys_sigreturn
 *    INPUT: None
 * FUNCTION: empty handler, returns error (-1)
 *           if implemented, returns from signal
 */
int32_t sys_sigreturn (void)
{
	return ERROR;
}

/* sys_zero
 *    INPUT: none
 * FUNCTION: emtpy handler for syscall 0 , returns 0
 */
int32_t sys_zero(void)
{
	return 0;
}


/*---------- HELPER FUNCTIONS ----------*/

/* process_start_file_d
 *    INPUT:
 * FUNCTION:
 */
void process_start_file_d(file_d *process_fds)
{
	int i;
		// stdin
	process_fds[STDIN].file_d_jump = stdin_jump;
	process_fds[STDIN].inode_num = NULL; // not data file
	process_fds[STDIN].file_pos = FILE_START;
	process_fds[STDIN].flag = FLAG_SET; // in use
		// stdout  
	process_fds[STDOUT].file_d_jump = stdout_jump;
	process_fds[STDOUT].inode_num = NULL; // not data file
	process_fds[STDOUT].file_pos = FILE_START;
	process_fds[STDOUT].flag = FLAG_SET; // in use

	for(i = FIRST_NON_STD_FD; i < TOTAL_NUMBER_OF_FILE_DESCRIPTORS; i++ )
		process_fds[i].flag = FLAG_UNSET;

}

/* get_pcb_ptr
 *    INPUT: None
 * FUNCTION: get pointer to pcb coor. to current kernel stack
 */
PCB* get_pcb_ptr(void)
{
	uint32_t MASK = 0xFFFFE000;
	PCB* pcb_addr = NULL;
	asm volatile("andl %%esp, %%eax;"
			 : "=a"(pcb_addr)
			 : "a"(MASK)
			 : "cc"
			 );
	return pcb_addr;
}

/* get_available_fd
 *    INPUT: return_file_descriptor_index - 
 * FUNCTION:
 */
file_d* get_available_fd(uint32_t* return_file_descriptor_index)
{
	PCB* current_pcb = get_pcb_ptr();
	uint32_t file_descriptor_index; //stdin and stdout always take spots 0 and 1
	for(file_descriptor_index=FIRST_NON_STD_FD;file_descriptor_index<TOTAL_NUMBER_OF_FILE_DESCRIPTORS; file_descriptor_index++)
	{
		//flag 0 for free, 1 for in use
		if(!current_pcb->fd[file_descriptor_index].flag)
		{
			*return_file_descriptor_index = file_descriptor_index;
			return &(current_pcb->fd[file_descriptor_index]);
		}
	}
	*return_file_descriptor_index = ERROR;
	return NULL;
	
}
