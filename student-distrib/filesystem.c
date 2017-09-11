#include "filesystem.h"

/* initialize_file_system
 *    INPUT:
 * FUNCTION:
 */
void initialize_file_system(void * file_system_start_address)
{
	boot_block = (file_system_statistics_t *)file_system_start_address;
}

/* read_dentry_by_name
 *    INPUT:
 * FUNCTION:
 */
int32_t read_dentry_by_name(const uint8_t* filename, dentry_t* dentry)
{
	if(dentry == NULL)
		return -1;
	uint32_t dentry_index = 0;
    int len_input = strlen((int8_t *)filename);
    int len_file, length;
    dentry_t * searched_dentry = (dentry_t *)(boot_block);
    searched_dentry += FIRST_FILE_OR_FOLDER_OFFSET;

    for(dentry_index = 0; dentry_index < boot_block->num_dentries; dentry_index++) {
    	len_file = strlen((int8_t*)searched_dentry->file_name);
    	length = (len_file > len_input) ? len_file : len_input;
    	length = (length > 32) ? 32 : length;
    	if(strncmp((int8_t*)searched_dentry->file_name, (int8_t *)filename, length) == 0) {
    		memcpy(dentry, searched_dentry, sizeof(dentry_t));
    		return 0;
    	}
    	searched_dentry += 1;
    }
	return -1;
}

/* read_dentry_by_index
 *    INPUT:
 * FUNCTION:
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	if(index < 0 || index >= boot_block->num_inodes || dentry == NULL)
	{
		return -1;
	}
	dentry_t * searched_dentry = (dentry_t *)(boot_block);
	searched_dentry += FIRST_FILE_OR_FOLDER_OFFSET;
	uint32_t inode_index = 0;
	for(inode_index = 0; inode_index < boot_block->num_inodes; inode_index++)
	{
		if(searched_dentry->inode_number == index)
		{
			memcpy(dentry, searched_dentry, sizeof(dentry_t));
			return 0;
		}
		searched_dentry++;
	}
	return -1;
}

/* read_data
 *    INPUT:
 * FUNCTION:
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
	if(inode < 0 || inode >= boot_block->num_inodes || buf == NULL || length <= 0)
		return -1;

	//GO TO INODE
	uint8_t * find_inode = (uint8_t *)(boot_block);
	find_inode += BLOCK_SIZE; //jump past the boot block
	find_inode += inode * BLOCK_SIZE; //select the correct inode
	inode_t * selected_inode = (inode_t *)find_inode;
	//GO TO INODE

	if(selected_inode->length_in_bytes <= 0)
	{
		return 0;
	}

	//CALCULATE INODE DATA BLOCK OFFSET
	uint32_t inode_dblock_offset = offset / BLOCK_SIZE;
	uint32_t dblock_offset = offset % BLOCK_SIZE;
	uint32_t curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
	//CALCULATE INODE DATA BLOCK OFFSET

	if(curr_dblock < 0)
		return -1;

	//FIND DATA BLOCK ZERO
	dblock_t * dblock_zero = (dblock_t *)boot_block;
	dblock_zero += INODE_OFFSET_IN_BLOCKS;
	dblock_zero += boot_block->num_inodes;
	//FIND DATA BLOCK ZERO

	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET
	uint8_t * curr_dblock_ptr = (uint8_t *)dblock_zero;
	curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
	curr_dblock_ptr += dblock_offset;
	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET

	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
	uint32_t data_byte_index = 0;
	uint32_t bytes_copied = 0;
	uint32_t remaining_length = 0;
	if(length > selected_inode->length_in_bytes - offset)
		remaining_length = selected_inode->length_in_bytes - offset;
	else
		remaining_length = length;

	if(offset > selected_inode->length_in_bytes)
		return 0; //EOF CONDITION OR ERROR CONDITION?

	for(data_byte_index = 0; data_byte_index < remaining_length; data_byte_index++) {
		if((offset+data_byte_index) % BLOCK_SIZE == 0 && data_byte_index !=0) {
			inode_dblock_offset++;
			if(inode_dblock_offset >= NUM_DBLOCK_REFS)
				return -1;
			curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
			curr_dblock_ptr = (uint8_t *)dblock_zero;
			curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
		}
		memcpy(buf, curr_dblock_ptr, 1);
		curr_dblock_ptr++;
		buf++;
		bytes_copied++;
	}
	return bytes_copied;
	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
}

/* read_data_corr_sig
 *    INPUT:
 * FUNCTION:
 */
int32_t read_data_corr_sig(uint32_t fd, uint8_t* buf, uint32_t length)
{
	PCB* pcb = get_pcb_ptr();
	uint32_t inode = pcb->fd[fd].inode_num;
	uint32_t offset = pcb->fd[fd].file_pos;
	if(inode < 0 || inode >= boot_block->num_inodes || buf == NULL || length <= 0)
		return -1;

	//GO TO INODE
	uint8_t * find_inode = (uint8_t *)(boot_block);
	find_inode += BLOCK_SIZE; //jump past the boot block
	find_inode += inode * BLOCK_SIZE; //select the correct inode
	inode_t * selected_inode = (inode_t *)find_inode;
	//

	if(selected_inode->length_in_bytes <= 0)
		return 0;

	//CALCULATE INODE DATA BLOCK OFFSET
	uint32_t inode_dblock_offset = offset / BLOCK_SIZE;
	uint32_t dblock_offset = offset % BLOCK_SIZE;
	uint32_t curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
	//

	if(curr_dblock < 0)
		return -1;

	//FIND DATA BLOCK ZERO
	dblock_t * dblock_zero = (dblock_t *)boot_block;
	dblock_zero += INODE_OFFSET_IN_BLOCKS;
	dblock_zero += boot_block->num_inodes;
	//

	//GET POINTER TO FIRST DATA BLOCK WITH OFFSET
	uint8_t * curr_dblock_ptr = (uint8_t *)dblock_zero;
	curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
	curr_dblock_ptr += dblock_offset;
	//

	//LOOP THROUGH THE INODE DATA BLOCKS COPYING MEMORY INTO BUF
	uint32_t data_byte_index = 0;
	uint32_t bytes_copied = 0;
	uint32_t remaining_length = 0;
	if(length >= selected_inode->length_in_bytes - offset)
		remaining_length = selected_inode->length_in_bytes - offset;
	else
		remaining_length = length;

	if(offset >= selected_inode->length_in_bytes)
		return 0; //EOF CONDITION OR ERROR CONDITION?
	for(data_byte_index = 0; data_byte_index < remaining_length; data_byte_index++) {
		if((offset+data_byte_index) % BLOCK_SIZE == 0 && data_byte_index !=0) {
			inode_dblock_offset++;
			if(inode_dblock_offset >= NUM_DBLOCK_REFS)
				return -1;
			curr_dblock = selected_inode->dblock_refs[inode_dblock_offset];
			curr_dblock_ptr = (uint8_t *)dblock_zero;
			curr_dblock_ptr += BLOCK_SIZE * curr_dblock;
		}
		memcpy(buf, curr_dblock_ptr, 1);
		curr_dblock_ptr++;
		buf++;
		bytes_copied++;
		pcb->fd[fd].file_pos++;
	}
	return bytes_copied;
	//
}

/* file_write
 *    INPUT:
 * FUNCTION:
 */
uint32_t file_write (int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/* directory_write
 *    INPUT:
 * FUNCTION:
 */
uint32_t directory_write (int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/* file_close
 *    INPUT:
 * FUNCTION:
 */
uint32_t file_close (int32_t fd)
{
	return 0;
}

/* directory_close
 *    INPUT:
 * FUNCTION:
 */
uint32_t directory_close (int32_t fd)
{
	return 0;
}

/* read_directory
 *    INPUT:
 * FUNCTION:
 */
uint32_t read_directory (int32_t fd, void* buf, int32_t nbytes)
{
	/*
	PCB* pcb = get_pcb_ptr();
	dentry_t * curr_dentry = (dentry_t *)(boot_block);
	curr_dentry += FIRST_FILE_OR_FOLDER_OFFSET;
	uint32_t dentry_index = pcb->fd[fd].file_pos;
	for(dentry_index = 0; dentry_index < boot_block->num_dentries; dentry_index++)
	{
		if(curr_dentry->file_type == REGULARFILETYPE)
		{
			int length = strlen((int8_t*)curr_dentry->file_name);
			stdout_write(0, curr_dentry->file_name, (length > 32) ? 32 : length);
			stdout_write(0, "\n", strlen("\n"));
		}
		curr_dentry += 1;
	}
	pcb->fd[fd].file_pos++;
	return 0;
	*/
	// get pcb to access fd
	PCB* pcb = get_pcb_ptr();
	uint32_t offset = pcb->fd[fd].file_pos;
	// check if offset reading blocks without file name
	if(boot_block->dentries[offset].file_name[0] == '\0')
		return 0;
	uint32_t length = 0;
	// as much as fits or all 32 bytes
	length = (nbytes > 32) ? 32 : nbytes;
	// copy to buffer
	strncpy((int8_t*) buf, (int8_t*)boot_block->dentries[offset].file_name, length);
	// update position for subsequent reads
	pcb->fd[fd].file_pos++;
	return length;
}

/* open_file
 *    INPUT:
 * FUNCTION:
 */
uint32_t open_file(const uint8_t * filename, dentry_t * dentry)
{
	return read_dentry_by_name(filename, dentry);
}

/* open_directory
 *    INPUT:
 * FUNCTION:
 */
uint32_t open_directory(const uint8_t * filename, dentry_t * dentry)
{
	return read_dentry_by_name(filename, dentry);
}
