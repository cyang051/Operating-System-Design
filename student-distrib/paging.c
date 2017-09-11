#include "paging.h"

/* paging_init
 * 	   INPUT: None
 *	FUNCTION: Enables paging and fills the page directories and page tables
 */
void paging_init(void)
{
	/*
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
	*/
	uint32_t i;
	// initialize all page directories/tables to not present
 	for(i = 0; i < NUM_PAGE_DIRECTORY_ENTRIES; i++) {
 		page_directory[i].val = 0;
 		page_directory[i].read_write = 1; // all pages read/write enabled
 	}
	 	
	for(i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++) {
		page_table[i].val = 0;
		user_video_page_table[i].val = 0;
		page_table[i].read_write = 1;	// all pages read/write enabled
		user_video_page_table[i].read_write = 1;
		page_table[i].address = i;	// assign address to page table
	}
	// assign 1 page to display (80 width * 25 height * 2 byte < 4096)
	page_table[VIDEO_MEM_LOCATION].present = 1;
	// assign table for 1st entry of directory
	page_directory[0].address = ((int)page_table) >> 12;	// shift by 12 to remove non-address bits
	page_directory[0].present = 1; 
	// assign kernel page
	page_directory[1].address = KERNEL_ADDRESS >> 12; // shift by 12 to remove non-address bits
	page_directory[1].global = 1;
	page_directory[1].size = 1;
	page_directory[1].cache_disabled = 1;
	page_directory[1].present = 1;
	// assign user video memory page
	user_video_page_table[0].address = VIDEO_MEM_ADDRESS >> 12;
	user_video_page_table[0].user_supervisor = 1;
	// user_video_page_table[0].read_write = 1;
	user_video_page_table[0].present = 1;
	page_directory[USER_VIDEO_LOCATION].address = ((int)user_video_page_table) >> 12;
	page_directory[USER_VIDEO_LOCATION].user_supervisor = 1;
	page_directory[USER_VIDEO_LOCATION].read_write = 1;
	page_directory[USER_VIDEO_LOCATION].present = 1;
	// enable paging
	asm (
		"movl	%%eax, %%cr3;"	// load cr3 with address of page directory
		"movl	%%cr4, %%eax;"	// enable 4 MB pages by turning PSE on in cr4 (bit 4, PSE)
		"orl	$0x00000010, %%eax;"
		"movl	%%eax, %%cr4;"
		"movl	%%cr0, %%eax;"	// enable paging by turning PG on in cr0 (bit 31) 
		"orl	$0x80000000, %%eax;"
		"movl	%%eax, %%cr0;"
		: // no outputs
		: "a"(page_directory)
		: "cc"
		);
}

/* paging_init
 * 	   INPUT: ???
 *	FUNCTION: ???
 */
void program_paging(uint32_t physical_address)
{
	page_directory[FIRST_PROGRAM_LOCATION].address = physical_address >> 12; // shift by 12 to remove non-address bits
	page_directory[FIRST_PROGRAM_LOCATION].size = 1;
	page_directory[FIRST_PROGRAM_LOCATION].user_supervisor = 1;
	page_directory[FIRST_PROGRAM_LOCATION].read_write = 1;
	page_directory[FIRST_PROGRAM_LOCATION].present = 1;

	// flush TLB
	asm("movl %%cr3, %%eax;"
		"movl %%eax, %%cr3;"
		: // no outputs
		: // no inputs
		: "memory", "cc", "eax"
		);
}
