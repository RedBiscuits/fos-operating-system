/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists()
{
	cprintf("\n=========================================\n");
	struct MemBlock* blk ;
	struct MemBlock* lastBlk = NULL ;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1 ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nFreeMemBlocksList is NOT SORTED!!\n") ;

	lastBlk = NULL ;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1 ;
	LIST_FOREACH(blk, &AllocMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nAllocMemBlocksList is NOT SORTED!!\n") ;
	cprintf("\n=========================================\n");
}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
struct MemBlock* nfIterator = NULL;
void initialize_MemBlocksList(uint32 numOfBlocks)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] initialize_MemBlocksList
	// Write your code here, remove the panic and write your code
	//panic("initialize_MemBlocksList() is not implemented yet...!!");
	LIST_INIT(&AvailableMemBlocksList);
//	AvailableMemBlocksList.___ptr_next = NULL;
	for (int i = 0; i < numOfBlocks; ++i) {
		LIST_INSERT_TAIL(&AvailableMemBlocksList , &MemBlockNodes[i]);
	}
}

//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] find_block
	// Write your code here, remove the panic and write your code
	//panic("find_block() is not implemented yet...!!");
	struct MemBlock* block = NULL;
	LIST_FOREACH(block , blockList ){
		if (block->sva == va){
			return block;
		}
	}
	return NULL;
}

//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] insert_sorted_allocList
	// Write your code here, remove the panic and write your code
	//panic("insert_sorted_allocList() is not implemented yet...!!");
	if(AllocMemBlocksList.size == 0){
		LIST_INSERT_HEAD(&AllocMemBlocksList , blockToInsert);
	}
	else{
		struct MemBlock *blk;
		int added = 0;
		LIST_FOREACH(blk , &AllocMemBlocksList ){
			if(blk->sva > blockToInsert->sva){
				LIST_INSERT_BEFORE(&AllocMemBlocksList , blk ,blockToInsert);
				added =1;
				break;
			}
		}
		if(added == 0){
			LIST_INSERT_TAIL(&AllocMemBlocksList , blockToInsert);
		}
	}
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
struct MemBlock *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] alloc_block_FF
	// Write your code here, remove the panic and write your code
	struct MemBlock* blk ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if(size == blk->size){
			LIST_REMOVE(&FreeMemBlocksList , blk);
			return blk;
		}else if(size < blk->size){
			struct MemBlock* newBlk ;
			newBlk = LIST_LAST(&AvailableMemBlocksList);
			newBlk->sva = blk->sva;
			newBlk->size = size;
			blk->sva += size;
			blk->size -= size;
			LIST_REMOVE(&AvailableMemBlocksList , newBlk);
			return newBlk;
		}
	}
	return NULL;
	//panic("alloc_block_FF() is not implemented yet...!!");
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
struct MemBlock *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT MS1] [DYNAMIC ALLOCATOR] alloc_block_BF
	// Write your code here, remove the panic and write your code
	//panic("alloc_block_BF() is not implemented yet...!!");

	uint32 Best_Fit = -1; //Initially large value ((2^32)-1) to catch any value that is lower than it.
	struct MemBlock* BF_Block = NULL; //Pointer to mark the BF block.
	struct MemBlock* Block = NULL; //Pointer to iterate the list.
		LIST_FOREACH(Block , &FreeMemBlocksList ){
			if (Block->size >= size && Block->size < Best_Fit) //Find the best fit.
			{
				Best_Fit = Block->size;
				BF_Block = Block;
				if(Best_Fit == size)
				{
					LIST_REMOVE(&FreeMemBlocksList , BF_Block);
					return BF_Block;
				}
			}
		}
	if(Best_Fit == -1) //There is no block that is big enough.
	{
		return NULL;
	}
	else //The free block is larger than the one to be allocated
	{
		//Make a block with exactly the requested size, and deduce that size from the best fitting block.
		struct MemBlock* newBlk ;
		newBlk = LIST_FIRST(&AvailableMemBlocksList);
		newBlk->sva = BF_Block->sva;
		newBlk->size = size;
		BF_Block->sva += size;
		BF_Block->size -= size;
		LIST_REMOVE(&AvailableMemBlocksList , newBlk);
		return newBlk;
	}
}


//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================

struct MemBlock *alloc_block_NF(uint32 size)
{
	if(nfIterator == NULL){
		nfIterator = alloc_block_FF(size);
		return nfIterator;
	}
	else{
		struct MemBlock* blk = nfIterator;
		LIST_FOREACH(blk ,&FreeMemBlocksList ){
			if(blk->sva >= nfIterator->sva){
				if(size == blk->size){
					LIST_REMOVE(&FreeMemBlocksList , blk);
					nfIterator =blk;
					return blk;
				}else if(size < blk->size){
					struct MemBlock* newBlk ;
					newBlk = LIST_FIRST(&AvailableMemBlocksList);
					newBlk->sva = blk->sva;
					newBlk->size = size;
					blk->sva += size;
					blk->size -= size;
					LIST_REMOVE(&AvailableMemBlocksList , newBlk);
					//AvailableMemBlocksList.size--; //TO PREVENT ERRORS
					nfIterator =newBlk;
					return newBlk;
				}
			}
		}
		LIST_FOREACH(blk ,&FreeMemBlocksList ){
			if(blk->sva <= nfIterator->sva){
				if(size == blk->size){
					LIST_REMOVE(&FreeMemBlocksList , blk);
					nfIterator =blk;
					return blk;
				}else if(size < blk->size){
					struct MemBlock* newBlk ;
					newBlk = LIST_FIRST(&AvailableMemBlocksList);
					newBlk->sva = blk->sva;
					newBlk->size = size;
					blk->sva += size;
					blk->size -= size;
					LIST_REMOVE(&AvailableMemBlocksList , newBlk);
					nfIterator =newBlk;
					return newBlk;
				}
			}else{break;}
		}
	}
	nfIterator =NULL;
	return NULL;
}


//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================
void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert)
{
	//cprintf("BEFORE INSERT with MERGE: insert [%x, %x)\n=====================\n", blockToInsert->sva, blockToInsert->sva + blockToInsert->size);
	//print_mem_block_lists() ;
	if (LIST_SIZE(&FreeMemBlocksList)== 0){
			LIST_INSERT_HEAD(&FreeMemBlocksList , blockToInsert);

			return;
		} else {
			struct MemBlock* block = NULL;
			LIST_FOREACH(block , &FreeMemBlocksList ){
					if (block->sva > blockToInsert->sva){

						int blockToInsertEndSva = blockToInsert->size + blockToInsert->sva;

						if(block->sva == LIST_FIRST(&FreeMemBlocksList)->sva)
						{
							if (block->sva == blockToInsertEndSva){
								blockToInsert->size += block->size;

								LIST_INSERT_HEAD(&FreeMemBlocksList , blockToInsert);
								block->size = 0;
								block->sva = 0;

								LIST_REMOVE(&FreeMemBlocksList , block);
								LIST_INSERT_HEAD(&AvailableMemBlocksList ,block);
							} else {
								LIST_INSERT_HEAD(&FreeMemBlocksList , blockToInsert);
							}
							return;
						}

						struct MemBlock* beforeBlock = block->prev_next_info.le_prev;
						int beforeEndSva = beforeBlock->size + beforeBlock->sva;

						//Merge with before and after
						if (beforeEndSva == blockToInsert->sva && blockToInsertEndSva == block->sva){

							blockToInsert->size += block->size + beforeBlock->size;
							blockToInsert->sva = beforeBlock->sva;

							LIST_INSERT_AFTER(&FreeMemBlocksList , beforeBlock , blockToInsert);

							beforeBlock->size = 0;
							beforeBlock->sva = 0;

							block->size = 0;
							block->sva = 0;

							LIST_REMOVE(&FreeMemBlocksList , beforeBlock);
							LIST_REMOVE(&FreeMemBlocksList , block);

							LIST_INSERT_HEAD(&AvailableMemBlocksList ,block);
							LIST_INSERT_HEAD(&AvailableMemBlocksList ,beforeBlock);
							return;


						//Merge with before
						} else if (beforeEndSva == blockToInsert->sva){

							blockToInsert->size += beforeBlock->size;
							blockToInsert->sva = beforeBlock->sva;

							LIST_INSERT_AFTER(&FreeMemBlocksList , beforeBlock , blockToInsert);

							beforeBlock->size = 0;
							beforeBlock->sva = 0;

							LIST_REMOVE(&FreeMemBlocksList , beforeBlock);
							LIST_INSERT_HEAD(&AvailableMemBlocksList ,beforeBlock);
							return;



						//Merge with after
						} else if (blockToInsertEndSva == block->sva){

							blockToInsert->size += block->size;

						    LIST_INSERT_BEFORE(&FreeMemBlocksList , block , blockToInsert);

						    block->size = 0;
						    block->sva = 0;

							LIST_REMOVE(&FreeMemBlocksList , block);
							LIST_INSERT_HEAD(&AvailableMemBlocksList ,block);
							return;


						// No Merge
						} else {
							LIST_INSERT_BEFORE(&FreeMemBlocksList ,block,blockToInsert);
							return;

						}

					}


				}
			struct MemBlock *tail = LIST_LAST(&FreeMemBlocksList);

			if(tail->sva + tail->size == blockToInsert->sva)
			{
				blockToInsert->size += tail->size;
				blockToInsert->sva = tail->sva;

				LIST_INSERT_AFTER(&FreeMemBlocksList , tail , blockToInsert);

				tail->size = 0;
				tail->sva = 0;

				LIST_REMOVE(&FreeMemBlocksList , tail);
				LIST_INSERT_HEAD(&AvailableMemBlocksList ,tail);
				return;

			}
			else
			{
				LIST_INSERT_TAIL(&FreeMemBlocksList,blockToInsert);
			}


			return;
		}

	// Write your code here, remove the panic and write your code



	//cprintf("\nAFTER INSERT with MERGE:\n=====================\n");
	//print_mem_block_lists();

}
