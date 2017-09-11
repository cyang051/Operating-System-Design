#ifndef _filesystem_H
#define _filesystem_H

#include "types.h"
#include "lib.h"
#include "drivers/terminal.h"

#define NUM_DBLOCK_REFS 1023
#define BLOCK_SIZE 4096
#define FIRST_FILE_OR_FOLDER_OFFSET 1
#define INODE_OFFSET_IN_BLOCKS 1
#define NUMBER_OF_DENTRIES_IN_BLOCK 64
#define	RTCFILETYPE 0
#define DIRECTORYFILETYPE 1
#define REGULARFILETYPE 2

typedef struct
{
	uint8_t file_name[32]; // 1 * 32 = 32
	uint32_t file_type; // 4
	uint32_t inode_number; // 4
	uint8_t reserved[24]; // 1 * 24 = 24
	// = 64
} dentry_t;

typedef struct
{
	uint32_t num_dentries; // 4
	uint32_t num_inodes; // 4
	uint32_t num_blocks; // 4
	uint8_t reserved[52]; // 1 * 52 = 52
	// = 64
	dentry_t dentries[63]; // 63 * 64 = 4032
	// = 4096
} file_system_statistics_t;

typedef struct
{
	uint32_t length_in_bytes; // 4
	uint32_t dblock_refs[NUM_DBLOCK_REFS]; // 4 * 1023
	// = 4096
} inode_t;

typedef struct
{
	uint8_t data[BLOCK_SIZE]; // 1 * 4096 = 4096
	// = 4096
} dblock_t;

uint32_t open_file(const uint8_t * filename, dentry_t * dentry);
int32_t read_dentry_by_name(const uint8_t* filename, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
uint32_t file_write (int32_t fd, const void* buf, int32_t nbytes);
uint32_t file_close (int32_t fd);
void initialize_file_system(void * file_system_start_address);
uint32_t open_directory(const uint8_t * filename, dentry_t * dentry);
uint32_t read_directory (int32_t fd, void* buf, int32_t nbytes);
uint32_t directory_write (int32_t fd, const void* buf, int32_t nbytes);
uint32_t directory_close (int32_t fd);
int32_t read_data_corr_sig(uint32_t fd, uint8_t* buf, uint32_t length);


file_system_statistics_t * boot_block;

#endif
