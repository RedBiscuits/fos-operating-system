/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"


/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/


int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] cut_paste_pages
	// Write your code here, remove the panic and write your code
	//panic("cut_paste_pages() is not implemented yet...!!");
	uint32 source_va_aligned = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 dest_va_aligned = ROUNDDOWN(dest_va, PAGE_SIZE);


	//if a dest page has a frame (page exists), return -1;
	uint32 dest_va_iter = dest_va_aligned;
	uint32 *dest_va_table = NULL;

	for(int i = 0; i < num_of_pages; i++)
	{
		if (get_frame_info(page_directory, dest_va_iter, &dest_va_table) != NULL)
		{
			return -1;
		}
		dest_va_iter += PAGE_SIZE;
	}

	//cut paste functionality:
	uint32 *source_va_table = NULL;
	uint32 source_va_iter = source_va_aligned;
	dest_va_iter = dest_va_aligned;
	uint32 source_page_entry;
	struct FrameInfo* source_frame_info = NULL;

	for(int i = 0; i < num_of_pages; i ++)
	{
		//get the frame to map and page table corresponding to the page entry
		source_frame_info = get_frame_info(page_directory, source_va_iter, &source_va_table);

		//get the entry to get the control bits to map them too
		source_page_entry = source_va_table[PTX(source_va_iter)];

		//cut-paste, basically mapping (to destination) and unmapping (from source) the frames:
		map_frame(page_directory, source_frame_info, dest_va_iter, source_page_entry);
		unmap_frame(page_directory, source_va_iter);
		dest_va_iter += PAGE_SIZE;
		source_va_iter += PAGE_SIZE;
	}
	return 0;
}
//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va
//if the page table at any destination page in the range is not exist, it should create it
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] copy_paste_chunk
		// Write your code here, remove the panic and write your code
		//panic("copy_paste_chunk() is not implemented yet...!!");
		int chunk_end =ROUNDUP(dest_va+size,PAGE_SIZE);
	    struct FrameInfo* source_frame_info = NULL;
		struct FrameInfo* dest_frame_info = NULL;
		int table_exists_ret=0;
		uint32 source_va2 = ROUNDDOWN(source_va,PAGE_SIZE);

		for(uint32 i = ROUNDDOWN(dest_va,PAGE_SIZE); i < chunk_end; i+=PAGE_SIZE){
			  uint32 *dest_va_table=NULL;
			  get_page_table(page_directory, i, &dest_va_table);

				if (get_frame_info(page_directory, i, &dest_va_table)!=NULL){

					if((pt_get_page_permissions(page_directory,i) & PERM_WRITEABLE)==0){
					return -1;

					}
				}
			}

		for(uint32 i = ROUNDDOWN(dest_va,PAGE_SIZE); i < chunk_end; i+=PAGE_SIZE){
			uint32 *ptr_page_table = NULL;
			get_page_table(page_directory, i, &ptr_page_table);

			if(get_frame_info(page_directory, i, &ptr_page_table)==NULL){

				allocate_frame(&dest_frame_info);
				int ret = pt_get_page_permissions(page_directory,source_va2) & PERM_USER;
				map_frame(page_directory, dest_frame_info, i, PERM_WRITEABLE | ret);
			}
			source_va2+=PAGE_SIZE;
		}

		char * first = NULL,*second = NULL;
		for(int i = 0; i < size; i++){

			first =(char*)source_va;
			second=(char*)dest_va;
		    second[i]=first[i];

		}

		return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should share the given size from dest_va with the source_va
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] share_chunk
	// Write your code here, remove the panic and write your code
	//panic("share_chunk() is not implemented yet...!!");

	uint32 source_va_aligned = ROUNDDOWN(source_va, PAGE_SIZE);
	uint32 dest_va_aligned = ROUNDDOWN(dest_va, PAGE_SIZE);

	int chunk_end =ROUNDUP(source_va+size, PAGE_SIZE);

	//if a dest page has a frame (page exists), return -1;
	uint32 dest_va_iter = dest_va_aligned;
	uint32 *dest_va_table = NULL;


	for(uint32 i = source_va_aligned; i < chunk_end; i += PAGE_SIZE)
	{
		if (get_frame_info(page_directory, dest_va_iter, &dest_va_table) != NULL)
		{
			return -1;
		}
		dest_va_iter += PAGE_SIZE;
	}

	//share functionality:
	uint32 *source_va_table = NULL;
	uint32 source_va_iter = source_va_aligned;
	dest_va_iter = dest_va_aligned;
	uint32 source_page_entry;
	struct FrameInfo* source_frame_info = NULL;

	for(uint32 i = source_va_aligned; i < chunk_end; i += PAGE_SIZE)
	{
		//get the frame to map and page table corresponding to the page entry
		source_frame_info = get_frame_info(page_directory, source_va_iter, &source_va_table);

		//get the entry to get the control bits to map them too
		source_page_entry = source_va_table[PTX(source_va_iter)];
//
//		//check if destination address has a page table or not, and create it if missing:
//		table_exists_ret = get_page_table(page_directory, dest_va_iter, &ptr_page_table);
//		if (table_exists_ret == TABLE_NOT_EXIST)
//		{
//			create_page_table(page_directory, dest_va_iter);
//		}

		//share functionality, basically just mapping dest va so source frames
		map_frame(page_directory, source_frame_info, dest_va_iter, perms);
		dest_va_iter += PAGE_SIZE;
		source_va_iter += PAGE_SIZE;
	}
	return 0;

}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate in RAM the given range [va, va+size)
//Hint: use ROUNDDOWN/ROUNDUP macros to align the addresses
//int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
//{
//	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] allocate_chunk
//	// Write your code here, remove the panic and write your code
//	panic("allocate_chunk() is not implemented yet...!!");
//}
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	//TODO: [PROJECT MS2] [CHUNK OPERATIONS] allocate_chunk
	// Write your code here, remove the panic and write your code
	uint32 endOfChunk = va+size;
	int endOfPage =ROUNDUP(endOfChunk, PAGE_SIZE);
	uint32 *ptr_page_table = NULL;
	struct FrameInfo* ptr_frame_info;

	for(uint32 va_aligned = ROUNDDOWN(va,PAGE_SIZE); va_aligned < endOfPage;va_aligned = va_aligned + PAGE_SIZE)
	{
		ptr_frame_info = get_frame_info(page_directory, va_aligned, &ptr_page_table);
		if (ptr_frame_info != NULL){ return -1;}
	}

	for(uint32 va_aligned = ROUNDDOWN(va,PAGE_SIZE); va_aligned < endOfPage;va_aligned = va_aligned + PAGE_SIZE)
	{
		struct FrameInfo* ptr_frame_info = NULL;
		allocate_frame(&ptr_frame_info);
		map_frame(page_directory, ptr_frame_info, va_aligned, perms);
		ptr_frame_info->va = va_aligned;
	}
	return 0;
}

/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	//TODO: [PROJECT MS2 - BONUS] [CHUNK OPERATIONS] calculate_allocated_space
	// Write your code here, remove the panic and write your code
	//panic("calculate_allocated_space() is not implemented yet...!!");

	uint32 count_tables = 0;
	uint32 count_pages = 0;

	uint32 start = ROUNDDOWN(sva, PAGE_SIZE);
	uint32 end = ROUNDUP(eva, PAGE_SIZE);

	uint32 num_of_pages = (end-start)/PAGE_SIZE;

	uint32 *ptr_page_table = NULL;

	int table_id_record = -1;

	for(int i = 0; i < num_of_pages; i++)
	{
		struct FrameInfo* ptr_frame_info = get_frame_info(page_directory, start, &ptr_page_table);
		if (ptr_page_table != NULL && table_id_record != ptr_page_table[PDX(i)])
		{
			count_tables++;
			table_id_record = ptr_page_table[PDX(i)];

		}
		if(ptr_frame_info != NULL)
		{
			count_pages++;
		}
		start+= PAGE_SIZE;
	}

	*num_tables = count_tables;
	*num_pages = count_pages;

}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
// calculate_required_frames:
// calculates the new allocation size required for given address+size,
// we are not interested in knowing if pages or tables actually exist in memory or the page file,
// we are interested in knowing whether they are allocated or not.
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	//TODO: [PROJECT MS2 - BONUS] [CHUNK OPERATIONS] calculate_required_frames
	// Write your code here, remove the panic and write your code
	// panic("calculate_required_frames() is not implemented yet...!!");

	uint32 start = ROUNDDOWN(sva, PAGE_SIZE);
	uint32 end = ROUNDUP(sva + size, PAGE_SIZE);

	uint32 num_of_pages = (end-start)/PAGE_SIZE;

	uint32 count = 0;

	uint32 *ptr_page_table = NULL;


	for(int i = 0; i < num_of_pages; i++)
	{
		struct FrameInfo* ptr_frame_info = get_frame_info(page_directory, start, &ptr_page_table);

		if(ptr_frame_info == NULL)
		{
			count++;
		}

		start+= PAGE_SIZE;
	}

	start = ROUNDDOWN(sva, PAGE_SIZE* 1024);

	for(int i = start; i < end; i +=  PAGE_SIZE * 1024)
	{
		get_page_table(page_directory, i ,&ptr_page_table);
		if(ptr_page_table == NULL)
		{
			count++;

		}
	}

	return count;


}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3] [USER HEAP - KERNEL SIDE] free_user_mem

	//Variable declartions:
	uint32 sva = ROUNDDOWN(virtual_address ,PAGE_SIZE);
	uint32 endva = ROUNDUP((sva+size), PAGE_SIZE);
	uint32 *p_tbl = NULL;
	uint32 tbl_state;
	uint32 t_val;


	//Remove all pages from the page file.
	uint32 iter = sva;
	while (iter<= endva)
	{
		pf_remove_env_page(e, iter);
		iter+= PAGE_SIZE;
	}

	//Remove pages in the working set that are in this range.
	uint32 i = 0;
	for(;i<e->page_WS_max_size;i++)
	{tbl_state =env_page_ws_get_virtual_address(e,i);
	if (tbl_state >= sva && tbl_state < endva){//Unmap physical memory then clear working set entry.
	env_page_ws_clear_entry(e,i);unmap_frame(e->env_page_directory,tbl_state);}}

	int e_tbl =0; //Flag to mark whether the table is empty or not.
	uint32 start = ROUNDDOWN(sva, PAGE_SIZE * 1024); //VA of first page table.

	//Remove page tables that are empty.
	for(uint32 i = start; i< endva;i+=PAGE_SIZE * 1024)
	{
		p_tbl = NULL;
		get_page_table(e->env_page_directory,i,&p_tbl);
		//cprintf("i is: %x and p_tbl is: %x\n",i, (uint32)p_tbl);
		if(p_tbl != NULL) //If table exists
		{
			for(uint32 j = 0; j<1024;j++){
				t_val = p_tbl[j];
				if(0x0000 != t_val)
				{e_tbl =1;break;
				}
			}
			if(0x0000 == e_tbl ) //If table is empty (no entries exist)
			{
				int y =9;
				//Remove the empty page table.
				y++;
				//unmap_frame(e->env_page_directory, (uint32) p_tbl);
				kfree((void*)p_tbl);
				--y;
				//Clear entry from directory and flush the tlb cache to clear the translation of that entry if it exists.
				e->env_page_directory[PDX(i)] = 0;
				tlbflush();
			}
			e_tbl = 0;
		}
	}
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Free any BUFFERED pages in the given range
	//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//
