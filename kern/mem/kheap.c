#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

//==================================================================//
//==================================================================//
//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)//
//==================================================================//
//==================================================================//

void initialize_dyn_block_system()
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] initialize_dyn_block_system
	/*[2] Dynamically allocate the array of MemBlockNodes
	 * 	remember to:
	 * 		1. set MAX_MEM_BLOCK_CNT with the chosen size of the array
	 * 		2. allocation should be aligned on PAGE boundary
	 * 	HINT: can use alloc_chunk(...) function
	 */

	//[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList) [Hint: use LIST_INIT()]
	uint32 memsSize=0;
	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);

#if STATIC_MEMBLOCK_ALLOC
	//DO NOTHING
#else
	/*[2] Dynamically allocate the array of MemBlockNodes
	 * 	remember to:
	 * 		1. set MAX_MEM_BLOCK_CNT with the chosen size of the array
	 * 		2. allocation should be aligned on PAGE boundary
	 * 	HINT: can use alloc_chunk(...) function
	 */

	MAX_MEM_BLOCK_CNT = NUM_OF_KHEAP_PAGES;
	memsSize = ROUNDUP(MAX_MEM_BLOCK_CNT * sizeof(struct MemBlock) , PAGE_SIZE);
	allocate_chunk(ptr_page_directory, KERNEL_HEAP_START , memsSize , PERM_WRITEABLE);
	MemBlockNodes = (struct MemBlock *) KERNEL_HEAP_START;

#endif
	//[3] Initialize AvailableMemBlocksList by filling it with the MemBlockNodes
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
	//[4] Insert a new MemBlock with the remaining heap size into the FreeMemBlocksList

	struct MemBlock* freeMemBlock = LIST_FIRST(&AvailableMemBlocksList);
	freeMemBlock->size = KERNEL_HEAP_MAX - KERNEL_HEAP_START - memsSize;
	freeMemBlock->sva = KERNEL_HEAP_START + memsSize;
	LIST_REMOVE(&AvailableMemBlocksList , freeMemBlock);
	LIST_INSERT_HEAD(&FreeMemBlocksList , freeMemBlock);
}

void* handleFF(struct MemBlock* blk, uint32 SizeMazabot){
	blk = alloc_block_FF(SizeMazabot);
	if(NULL!= blk)
	{
		uint32 res = allocate_chunk(ptr_page_directory, blk->sva ,
				SizeMazabot , PERM_WRITEABLE);
		if(res == -1){return NULL;
		}
		else{insert_sorted_allocList(blk);
			return (void*)blk->sva;
		}
	}
	else{return NULL;
	}
}

void* handleBF(struct MemBlock* blk, uint32 SizeMazabot){
	blk = alloc_block_BF(SizeMazabot);
	if(NULL!= blk)
	{
		uint32 res = allocate_chunk(ptr_page_directory, blk->sva ,
				SizeMazabot , PERM_WRITEABLE);
		if(res == -1){return NULL;
		}
		else{insert_sorted_allocList(blk);
			return (void*)blk->sva;
		}
	}
	else{return NULL;
	}
}

void* handleNF(struct MemBlock* blk, uint32 SizeMazabot){
	blk = alloc_block_NF(SizeMazabot);
	if(NULL!= blk)
	{
		uint32 res = allocate_chunk(ptr_page_directory, blk->sva ,
		SizeMazabot , PERM_WRITEABLE);
		if(res == -1){return NULL;
		}
		else{insert_sorted_allocList(blk);return (void*)blk->sva;
		}
	}
	else{return NULL;
	}
}


void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kmalloc
	uint32 sizeAlligned = ROUNDUP(size, PAGE_SIZE);
	struct MemBlock* block =NULL;
	return (isKHeapPlacementStrategyFIRSTFIT())?handleFF(block, sizeAlligned):
				(isKHeapPlacementStrategyBESTFIT())?handleBF(block, sizeAlligned):
						(isKHeapPlacementStrategyNEXTFIT())?handleNF(block, sizeAlligned):NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kfree
	// Write your code here, remove the panic and write your code
	//get block from allocated blocks
	struct MemBlock* memBlock = find_block(&AllocMemBlocksList , (uint32)  virtual_address);

	//if block is null it's already free -- break (for last 25% eval)
	if(memBlock == NULL)
    	return ;

	//looping counter wise allocate chunk
    uint32 start = ROUNDDOWN((uint32)virtual_address , PAGE_SIZE);
    uint32 end = start + memBlock->size;
    for(uint32 i = start ; i < end ; i+=PAGE_SIZE){
    	unmap_frame(ptr_page_directory , i);
    }

    //removing block from allocated blocks and
    //inserting it with compaction into free list
    LIST_REMOVE(&AllocMemBlocksList , memBlock);
    insert_sorted_with_merge_freeList(memBlock);
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_virtual_address
	// Write your code here, remove the panic and write your code

	struct FrameInfo* frames = frames_info;
	struct FrameInfo physFrame = frames[physical_address >> 12];
	return physFrame.va;
//return the virtual address corresponding to given physical_address

	//refer to the project presentation and documentation for details
	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT MS2] [KERNEL HEAP] kheap_physical_address
	// Write your code here, remove the panic and write your code
	uint32* ptr;
	get_page_table(ptr_page_directory , virtual_address , &ptr);
	int framePA = ptr[PTX(virtual_address)];

	//return the physical address corresponding to given virtual_address
	return framePA & 0xFFFFF000;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT MS2 - BONUS] [KERNEL HEAP] krealloc
	// Write your code here, remove the panic and write your code
	new_size = ROUNDUP(new_size , PAGE_SIZE);
	virtual_address = (void*)ROUNDDOWN((uint32)virtual_address , PAGE_SIZE);
	if(new_size == 0){
		kfree(virtual_address);
		return NULL;
	}
	if(virtual_address == NULL){
		return kmalloc(new_size);
	}
	void* new = NULL;
	if((uint32)virtual_address >= KERNEL_HEAP_START){
		struct MemBlock* memBlock = find_block(&AllocMemBlocksList , (uint32)  virtual_address);
		struct MemBlock copy;
		if(memBlock == NULL){
			return NULL;
		}
		if(memBlock->size == new_size){
			return virtual_address;
		}
		if(new_size < memBlock->size ){
			kfree(virtual_address + new_size);
			return virtual_address;
		}
		if(new_size > memBlock->size){
			uint32 diff = new_size - memBlock->size;
			struct MemBlock* addBlock = find_block(&FreeMemBlocksList , (uint32)virtual_address + memBlock->size);
			if(addBlock != NULL){
				if(addBlock->size >= diff && addBlock->sva == (memBlock->sva + memBlock->size)){
					int perms = pt_get_page_permissions(ptr_page_directory , (memBlock->sva + (memBlock->size / 2)));
					allocate_chunk(ptr_page_directory, memBlock->sva + memBlock->size, diff , perms);
					memBlock->size += diff;
					addBlock->sva += diff;
					addBlock->size -= diff;
					return virtual_address;
				}
			}
		}
		new = kmalloc(new_size);
		cut_paste_pages(ptr_page_directory ,(uint32)virtual_address,(uint32)new, ROUNDUP(new_size /PAGE_SIZE, PAGE_SIZE));
		kfree(virtual_address);
	}

	return new;

}
