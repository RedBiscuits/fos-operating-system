/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//TODO: [PROJECT MS3] [FAULT HANDLER] page_fault_handler
	// Write your code here, remove the panic and write your code
		uint32 fault_va_aligned = ROUNDDOWN(fault_va, PAGE_SIZE);


		if(env_page_ws_get_size(curenv) < curenv->page_WS_max_size) //placement
		{
			//cprintf("here %x \n", fault_va);
			//Allocate physical space in memory for page
			struct FrameInfo* ptr_frame_info = NULL;
			allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, fault_va_aligned, PERM_USER | PERM_WRITEABLE);


			//Read the faulted page from disk (page file)
			int ret = pf_read_env_page(curenv, (void*)fault_va_aligned);

			if (ret == E_PAGE_NOT_EXIST_IN_PF) //Page doesn't exist on disk
			{
				if (fault_va > USTACKTOP || fault_va < USER_HEAP_START){
					//Not stack or heap page, panic!

					//Unmap frame in this case
					unmap_frame(curenv->env_page_directory, fault_va_aligned);
					panic("ILLEGAL MEM ACCESS");
				}

			}

			//Make sure last index is empty:

			while (1 == 1)
			{
				if(env_page_ws_is_entry_empty(curenv, curenv->page_last_WS_index)) //If empty break, else increment last_index till you find empty place.
				{
					break;
				}
				else{
					//Increment last index in a circular manner.
					curenv->page_last_WS_index = (curenv->page_last_WS_index + 1)% curenv->page_WS_max_size;

				}

			}

			//Place the faulted page in WS
			env_page_ws_set_entry(curenv, curenv->page_last_WS_index, fault_va_aligned);

			//Increment last index in a circular manner.
			curenv->page_last_WS_index = (curenv->page_last_WS_index + 1)% curenv->page_WS_max_size;


		}
		else //Replacement
		{
			//cprintf("there %x \n", fault_va);
			uint32 victim_index = 0;
			uint32 virtual_address = 0;
			uint32 perm = 0;
			//Find Victim in working set using clock algorithm
			while(1 == 1) //Loop till a victim is found
			{

				victim_index = curenv->page_last_WS_index;
				virtual_address = curenv->ptr_pageWorkingSet[victim_index].virtual_address;
				perm = pt_get_page_permissions(curenv->env_page_directory, virtual_address);
				if(perm & PERM_USED) //Used bit is 1.
				{
					//Clear PERM_USED (make it equal to 0)
					pt_set_page_permissions(curenv->env_page_directory, virtual_address, 0 , PERM_USED);
					curenv->page_last_WS_index = (curenv->page_last_WS_index + 1)% curenv->page_WS_max_size;
				}
				else{
					//The loop breaks and i stores the index for the victim to be replaced.
					//virtual_address stores the va of this page and perm stores the permissions associated with it


					//Update last index:
					curenv->page_last_WS_index = (curenv->page_last_WS_index + 1)% curenv->page_WS_max_size;

					break;
				}

			}


			//If victim is modified then update it in the page file (disk)

			if(perm & PERM_MODIFIED) //The victim is modified
			{
				//Update it to page file.
				uint32 *ptr_page_table = NULL;
				struct FrameInfo* ptr_frame_info = get_frame_info(curenv->env_page_directory, virtual_address, &ptr_page_table);
				pf_update_env_page(curenv, virtual_address, ptr_frame_info);
			}

			//Remove victim from working set and reflect changes

			//Remove entry:
			env_page_ws_clear_entry(curenv, victim_index);

			//Proceed with placement of faulted page in working set

			//Allocate physical space in memory for page
			struct FrameInfo* ptr_frame_info = NULL;
			allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, fault_va_aligned, PERM_USER | PERM_WRITEABLE);



			//Read the faulted page from disk (page file)
			int ret = pf_read_env_page(curenv, (void*)fault_va_aligned);

			if (ret == E_PAGE_NOT_EXIST_IN_PF) //Page doesn't exist on disk
			{
				if (fault_va > USTACKTOP || fault_va < USER_HEAP_START){
					//Not stack or heap page, panic!

					//Unmap frame in this case
					unmap_frame(curenv->env_page_directory, fault_va_aligned);
					panic("ILLEGAL MEM ACCESS");
				}



			}

			//Unmap frame of victim
			unmap_frame(curenv->env_page_directory, virtual_address);


			//Set at victim index.
			env_page_ws_set_entry(curenv, victim_index, fault_va_aligned);

		}

	//refer to the project presentation and documentation for details
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");


}
