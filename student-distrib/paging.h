#ifndef _paging_H
#define _paging_H
#include "types.h"
#define KiB4 4096
#define NUM_PAGE_DIRECTORY_ENTRIES 1024
#define NUM_PAGE_TABLE_ENTRIES 1024	
#define VIDEO_MEM_ADDRESS 0xB8000
#define VIDEO_MEM_LOCATION VIDEO_MEM_ADDRESS / 0x1000
#define KERNEL_ADDRESS 0x400000
#define FIRST_PROGRAM_VIRTUAL  0x08000000
#define USER_VIDEO_ADDRESS 0x08400000 // can be located anywher >= 132 MB
#define USER_VIDEO_LOCATION USER_VIDEO_ADDRESS / 0x400000
#define FIRST_PROGRAM_LOCATION FIRST_PROGRAM_VIRTUAL / 0x400000
typedef union page_directory_desc_t {
	uint32_t val;
	struct {
		uint8_t present : 1;
		uint8_t read_write : 1;
		uint8_t user_supervisor : 1;
		uint8_t write_through : 1;
		uint8_t cache_disabled : 1;
		uint8_t accessed : 1;
		uint8_t zero_bit : 1;
		uint8_t size : 1;
		uint8_t global : 1;
		uint8_t avail : 3;
		uint32_t address : 20;
	} __attribute__((packed));
} page_directory_desc_t;	
typedef union page_table_desc_t {
	uint32_t val;
	struct {
		uint8_t present : 1;
		uint8_t read_write : 1;
		uint8_t user_supervisor : 1;
		uint8_t write_through : 1;
		uint8_t cache_disabled : 1;
		uint8_t accessed : 1;
		uint8_t dirty : 1;
		uint8_t attribute : 1;
		uint8_t global : 1;
		uint8_t avail : 3;
		uint32_t address : 20;
	} __attribute__((packed));
} page_table_desc_t;
/* Initialize paging */
extern void paging_init(void);
extern void program_paging(uint32_t physical_address);

// Directory and table declaration
page_directory_desc_t page_directory[NUM_PAGE_DIRECTORY_ENTRIES] __attribute__((aligned(KiB4)));
page_table_desc_t page_table[NUM_PAGE_TABLE_ENTRIES] __attribute__((aligned(KiB4)));
page_table_desc_t user_video_page_table[NUM_PAGE_TABLE_ENTRIES] __attribute__((aligned(KiB4)));
#endif /* _paging_H */
