#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
		initialize_dyn_block_system();
		cprintf("DYNAMIC BLOCK SYSTEM IS INITIALIZED\n");
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//=================================
void initialize_dyn_block_system()
{
	//[1] Initialize two lists (AllocMemBlocksList & FreeMemBlocksList) [Hint: use LIST_INIT()]
	LIST_INIT(&AllocMemBlocksList);
	//TODO: [PROJECT MS3] [USER HEAP - USER SIDE] initialize_dyn_block_system
	LIST_INIT(&FreeMemBlocksList);
	//[2] Dynamically allocate the array of MemBlockNodes at VA USER_DYN_BLKS_ARRAY
	MAX_MEM_BLOCK_CNT = NUM_OF_UHEAP_PAGES;
	uint32 start = ROUNDDOWN(USER_DYN_BLKS_ARRAY , PAGE_SIZE);
	//	  (remember to set MAX_MEM_BLOCK_CNT with the chosen size of the array)
	uint32 	memsSize = ROUNDUP(MAX_MEM_BLOCK_CNT * sizeof(struct MemBlock) , PAGE_SIZE);
	sys_allocate_chunk(start,memsSize, PERM_WRITEABLE | PERM_USER);
	MemBlockNodes = (struct MemBlock *) start;
	//[3] Initialize AvailableMemBlocksList by filling it with the MemBlockNodes
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);

	//[4] Insert a new MemBlock with the heap size into the FreeMemBlocksList
	struct MemBlock* freeMemBlock = LIST_FIRST(&AvailableMemBlocksList);
	freeMemBlock->size = USER_HEAP_MAX - USER_HEAP_START;
	freeMemBlock->sva = USER_HEAP_START;
	LIST_REMOVE(&AvailableMemBlocksList , freeMemBlock);
	LIST_INSERT_HEAD(&FreeMemBlocksList , freeMemBlock);
//	USER_HEAP_START-USER_DYN_BLKS_ARRAY  -
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:

void* handle_block_value(struct MemBlock* block , uint32 sizeAlligned){
	block = alloc_block_FF(sizeAlligned);
	if(block != NULL){
		insert_sorted_allocList(block);
		return (void*)block->sva;}
	else{return NULL;
	}
}
void* malloc(uint32 size)
{
	InitializeUHeap();
	if (size == 0) return NULL ;
	struct MemBlock* memBLkNoDE =NULL;
	uint32 sizeMaZaBoot = ROUNDUP(size, PAGE_SIZE);
	return (sys_isUHeapPlacementStrategyFIRSTFIT())?
			handle_block_value(memBLkNoDE,sizeMaZaBoot):NULL;
	return NULL;
}

//=================================
// [3] FREE SPACE FROM USER HEAP:
void free(void* virtual_address)
{
	struct MemBlock* memBlock = find_block(&AllocMemBlocksList , (uint32)  virtual_address);
	//if block is null it's already free -- break (for last 25% eval)
	if(memBlock == NULL){return;
	}
	//looping counter wise allocate chunk
	uint32 start = ROUNDDOWN((uint32)virtual_address , PAGE_SIZE);
	sys_free_user_mem(start ,ROUNDUP(memBlock->size, PAGE_SIZE));
	//TODO: [PROJECT MS3] [USER HEAP - USER SIDE] free

	//removing block from allocated blocks and
	//inserting it with compaction into free list
	LIST_REMOVE(&AllocMemBlocksList , memBlock);
	insert_sorted_with_merge_freeList(memBlock);
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	InitializeUHeap();
	if (size == 0) {return NULL;
	}
	struct MemBlock* block =NULL;
	uint32 sizeAlligned = ROUNDUP(size, PAGE_SIZE);
	//chk algo
	if(sys_isUHeapPlacementStrategyFIRSTFIT()){
		//get blk
		block = alloc_block_FF(sizeAlligned);
		if(NULL!=block)
		{
	insert_sorted_allocList(block);
	int variableID = sys_createSharedObject(sharedVarName, size, isWritable, (void*)block->sva);
	return (variableID < 0)?NULL:(void*)block->sva;}
		else{return NULL;
		}
	}
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//TODO: [PROJECT MS3] [SHARING - USER SIDE] sget()
	//DoNT cHaNgE THat COoDE
	InitializeUHeap();
	//<><>><><><><><><><>
	struct MemBlock* block =NULL;
	uint32 sizeOfSharedObject = ROUNDUP(sys_getSizeOfSharedObject(ownerEnvID , sharedVarName), PAGE_SIZE );
	if (sizeOfSharedObject == E_SHARED_MEM_NOT_EXISTS){return NULL;
	}
	if(sys_isUHeapPlacementStrategyFIRSTFIT()){
		block = alloc_block_FF(sizeOfSharedObject);
		if (block == NULL){return NULL;
		}
		insert_sorted_allocList(block);
		int id_4_obj = sys_getSharedObject(ownerEnvID , sharedVarName , (void*)block->sva);
		return (id_4_obj == E_SHARED_MEM_NOT_EXISTS)?NULL:(void*)block->sva;
	} else {return NULL;
	}
}

void *realloc(void *virtual_address, uint32 new_size)
{

	InitializeUHeap();

	panic("realloc() is not implemented yet...!!");
}


void sfree(void* virtual_address)
{
	int32 identifierOfSharedObject = sys_getparentenvid();
//	virtual_address = ROUNDDOWN(virtual_address , PAGE_SIZE);
	sys_freeSharedObject(identifierOfSharedObject,virtual_address);
	free(virtual_address);

}

void expand(uint32 newSize)
{

	panic("Not Implemented");
}
void shrink(uint32 newSize)
{

	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{

	panic("Not Implemented");
}
