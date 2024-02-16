/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_set_page_permissions
	uint32 *ptr_to_page_table = NULL;
	int foundPageTable = get_page_table(page_directory, virtual_address, &ptr_to_page_table);
	if (foundPageTable == TABLE_IN_MEMORY)
	{
		ptr_to_page_table[PTX(virtual_address)] |= permissions_to_set;
		ptr_to_page_table[PTX(virtual_address)] &= ~permissions_to_clear;
		tlb_invalidate((void *)NULL, (void *)virtual_address);
	} else {
		panic("Invalid va");
	}
}

inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_get_page_permissions
	uint32 *ptr_to_page_table = NULL;
	int foundPageTable = get_page_table(page_directory, virtual_address, &ptr_to_page_table);
	if (foundPageTable == TABLE_IN_MEMORY)
	{
		return ptr_to_page_table[PTX(virtual_address)] & 0x00000FFF;
	} else {
		return -1;
	}
}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_clear_page_table_entry
	// Write your code here, remove the panic and write your code

	uint32 *ptr_to_page_table = NULL;

	int foundPageTable = get_page_table(page_directory, virtual_address, &ptr_to_page_table);

	if (foundPageTable == TABLE_IN_MEMORY)
	{
		ptr_to_page_table [PTX(virtual_address)] = 0;
		tlb_invalidate((void *)NULL, (void *)virtual_address);
	} else {
		panic("Invalid va");
	}
}

/***********************************************************************************************/

/*[2.2] ADDRESS CONVERTION*/
inline int virtual_to_physical(uint32* page_directory, uint32 virtual_address)
{
	uint32 *ptr_to_page_table = NULL;

	int foundPageTable = get_page_table(page_directory, virtual_address, &ptr_to_page_table);

	if (foundPageTable == TABLE_IN_MEMORY){

		struct FrameInfo * frameInfo = get_frame_info(page_directory , virtual_address , &ptr_to_page_table);

		return to_physical_address(frameInfo);


	} else {
		return -1;
	}
}

/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}
