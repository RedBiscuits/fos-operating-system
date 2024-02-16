#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//2017

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// [1] Create "shares" array:
//===========================
//Dynamically allocate the array of shared objects
//initialize the array of shared objects by 0's and empty = 1
void create_shares_array(uint32 numOfElements)
{
#if USE_KHEAP
	MAX_SHARES  = numOfElements ;
	shares = kmalloc(numOfElements*sizeof(struct Share));
	if (shares == NULL)
	{
		panic("Kernel runs out of memory\nCan't create the array of shared objects.");
	}
#endif
	for (int i = 0; i < MAX_SHARES; ++i)
	{
		memset(&(shares[i]), 0, sizeof(struct Share));
		shares[i].empty = 1;
	}
}

//===========================
// [2] Allocate Share Object:
//===========================
//Allocates a new (empty) shared object from the "shares" array
//It dynamically creates the "framesStorage"
//Return:
//	a) if succeed:
//		1. allocatedObject (pointer to struct Share) passed by reference
//		2. sharedObjectID (its index in the array) as a return parameter
//	b) E_NO_SHARE if the the array of shares is full (i.e. reaches "MAX_SHARES")
int allocate_share_object(struct Share **allocatedObject)
{
	int32 sharedObjectID = -1 ;
	for (int i = 0; i < MAX_SHARES; ++i)
	{
		if (shares[i].empty)
		{
			sharedObjectID = i;
			break;
		}
	}

	if (sharedObjectID == -1)
	{
		return E_NO_SHARE ;
/*		//try to increase double the size of the "shares" array
#if USE_KHEAP
		{
			shares = krealloc(shares, 2*MAX_SHARES);
			if (shares == NULL)
			{
				*allocatedObject = NULL;
				return E_NO_SHARE;
			}
			else
			{
				sharedObjectID = MAX_SHARES;
				MAX_SHARES *= 2;
			}
		}
#else
		{
			panic("Attempt to dynamically allocate space inside kernel while kheap is disabled .. ");
			*allocatedObject = NULL;
			return E_NO_SHARE;
		}
#endif
*/
	}

	*allocatedObject = &(shares[sharedObjectID]);
	shares[sharedObjectID].empty = 0;

#if USE_KHEAP
	{
		shares[sharedObjectID].framesStorage = create_frames_storage();
	}
#endif
	memset(shares[sharedObjectID].framesStorage, 0, PAGE_SIZE);

	return sharedObjectID;
}

//=========================
// [3] Get Share Object ID:
//=========================
//Search for the given shared object in the "shares" array
//Return:
//	a) if found: SharedObjectID (index of the shared object in the array)
//	b) else: E_SHARED_MEM_NOT_EXISTS
int get_share_object_ID(int32 ownerID, char* name)
{
	int i=0;

	for(; i< MAX_SHARES; ++i)
	{
		if (shares[i].empty)
			continue;

		//cprintf("shared var name = %s compared with %s\n", name, shares[i].name);
		if(shares[i].ownerID == ownerID && strcmp(name, shares[i].name)==0)
		{
			//cprintf("%s found\n", name);
			return i;
		}
	}
	return E_SHARED_MEM_NOT_EXISTS;
}

//=========================
// [4] Delete Share Object:
//=========================
//delete the given sharedObjectID from the "shares" array
//Return:
//	a) 0 if succeed
//	b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists
int free_share_object(uint32 sharedObjectID)
{
	if (sharedObjectID >= MAX_SHARES)
		return E_SHARED_MEM_NOT_EXISTS;

	//panic("deleteSharedObject: not implemented yet");
	clear_frames_storage(shares[sharedObjectID].framesStorage);
#if USE_KHEAP
	kfree(shares[sharedObjectID].framesStorage);
#endif
	memset(&(shares[sharedObjectID]), 0, sizeof(struct Share));
	shares[sharedObjectID].empty = 1;

	return 0;
}

// 2014 - edited in 2017
//===========================
// [5] Create frames_storage:
//===========================
// if KHEAP = 1: Create the frames_storage by allocating a PAGE for its directory
inline uint32* create_frames_storage()
{
	uint32* frames_storage = kmalloc(PAGE_SIZE);
	if(frames_storage == NULL)
	{
		panic("NOT ENOUGH KERNEL HEAP SPACE");
	}
	return frames_storage;
}
//===========================
// [6] Add frame to storage:
//===========================
// Add a frame info to the storage of frames at the given index
inline void add_frame_to_storage(uint32* frames_storage, struct FrameInfo* ptr_frame_info, uint32 index)
{
	uint32 va = index * PAGE_SIZE ;
	uint32 *ptr_page_table;
	int r = get_page_table(frames_storage,  va, &ptr_page_table);
	if(r == TABLE_NOT_EXIST)
	{
#if USE_KHEAP
		{
			ptr_page_table = create_page_table(frames_storage, (uint32)va);
		}
#else
		{
			__static_cpt(frames_storage, (uint32)va, &ptr_page_table);

		}
#endif
	}
	ptr_page_table[PTX(va)] = CONSTRUCT_ENTRY(to_physical_address(ptr_frame_info), 0 | PERM_PRESENT);
}

//===========================
// [7] Get frame from storage:
//===========================
// Get a frame info from the storage of frames at the given index
inline struct FrameInfo* get_frame_from_storage(uint32* frames_storage, uint32 index)
{
	struct FrameInfo* ptr_frame_info;
	uint32 *ptr_page_table ;
	uint32 va = index * PAGE_SIZE ;
	ptr_frame_info = get_frame_info(frames_storage,  va, &ptr_page_table);
	return ptr_frame_info;
}

//===========================
// [8] Clear the frames_storage:
//===========================
inline void clear_frames_storage(uint32* frames_storage)
{
	int fourMega = 1024 * PAGE_SIZE ;
	int i ;
	for (i = 0 ; i < 1024 ; i++)
	{
		if (frames_storage[i] != 0)
		{
#if USE_KHEAP
			{
				kfree((void*)kheap_virtual_address(EXTRACT_ADDRESS(frames_storage[i])));
			}
#else
			{
				free_frame(to_frame_info(EXTRACT_ADDRESS(frames_storage[i])));
			}
#endif
			frames_storage[i] = 0;
		}
	}
}


//==============================
// [9] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	// your code is here, remove the panic and write your code
	//panic("getSizeOfSharedObject() is not implemented yet...!!");

	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//

	int shareObjectID = get_share_object_ID(ownerID, shareName);
	if (shareObjectID == E_SHARED_MEM_NOT_EXISTS)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return shares[shareObjectID].size;

	return 0;
}

//********************************************************************************//

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=========================
// [1] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
{
	//TODO: [PROJECT MS3] [SHARING - KERNEL SIDE] createSharedObject()
	// your code is here, remove the panic and write your code
	//panic("createSharedObject() is not implemented yet...!!");

	uint32 sizeAlligned = ROUNDUP(size, PAGE_SIZE);
	uint32 vaAlligned = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
	struct Env* myenv = curenv; //The calling environment

	//if a shared object doesn't exist, make one, else return error

	int sharedObjectExists = get_share_object_ID(ownerID,shareName);
	if(sharedObjectExists == E_SHARED_MEM_NOT_EXISTS)
	{

			struct Share* allocatedObject;
			int sharedObjectID = allocate_share_object(&allocatedObject);
			//If there is no more space to make a new object return error, else continue with creation logic.
			if (sharedObjectID == E_NO_SHARE) {

				return E_NO_SHARE;
			}
			else{

				//Allocate physical memory
				allocate_chunk(curenv->env_page_directory, (uint32)virtual_address, sizeAlligned, PERM_WRITEABLE | PERM_USER);


				//Initialize values of the shared object from function parameters.
				allocatedObject->ownerID = ownerID;
				strcpy(allocatedObject->name,shareName);
				allocatedObject->size = size;
				allocatedObject->isWritable = isWritable;
				allocatedObject->references = 1;


				//Add each frame to the frame storage array of the shared object.
				uint32 *ptr_page_table = NULL;
				struct FrameInfo* ptr_frame_info;
				uint32 index = 0;

				for (uint32 i = vaAlligned; i < vaAlligned + sizeAlligned; i+= PAGE_SIZE)
				{
					ptr_frame_info = get_frame_info(curenv->env_page_directory, i, &ptr_page_table);
					add_frame_to_storage(allocatedObject->framesStorage, ptr_frame_info, index);

					index++;
				}

				//Finally, return shared object id
				return sharedObjectID;

			}
	}
	else{
		//Error
		return E_SHARED_MEM_EXISTS;
	}

	// This function should create the shared object at the given virtual address with the given size
	// and return the ShareObjectID
	// RETURN:
	//	a) ShareObjectID (its index in "shares" array) if success
	//	b) E_SHARED_MEM_EXISTS if the shared object already exists
	//	c) E_NO_SHARE if the number of shared objects reaches max "MAX_SHARES"
}

//======================
// [2] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address)
{
    //TODO: [PROJECT MS3] [SHARING - KERNEL SIDE] getSharedObject()
    // your code is here, remove the panic and write your code
    //panic("getSharedObject() is not implemented yet...!!");

	/*
	     *         PLEASE DON'T GIVE US A PLAGIARISM WE TRIED
	     *         THE SHIT OUT OF US IN THIS TRASHIC  PROJECT :c
	     *         WITH LOVE,
	     *         KAMMAR :)
	     * */

	    //bta3tk wallahy
	    struct Env* kammar = curenv; //The calling environment

	    //el page dir bdl el -> el kteer ya doc wallahy :(
	    uint32 *page_directory=kammar->env_page_directory;

	    // bngeeb el ID
	    int hamada=get_share_object_ID(ownerID,shareName);

	    // bntchek lw mafeesh share obj bnrg3 error
	    if(hamada==E_SHARED_MEM_NOT_EXISTS)
	            return E_SHARED_MEM_NOT_EXISTS;


	    // de mayada wel 7aga bta3t el loops *.*
	    struct Share * mayada1234=&shares[hamada];

	    // el size b3d ma etzbt 3la 3dd el pages
	    int natasha=ROUNDUP(mayada1234->size,PAGE_SIZE);

	    //3dd el pages 34an el loop
	    int asmaa_galal = natasha/PAGE_SIZE;

	    // hnlf w ndoor hena b2a
	    for(int khazoo2=0 ; khazoo2 < asmaa_galal ; ++khazoo2)
	    {
	        // de ex kimo f hn3ml beeha frame
	        struct FrameInfo* sally_fouad=get_frame_from_storage(mayada1234->framesStorage,khazoo2);
	        if(mayada1234->isWritable)
	        {

	            // bnmap ex kimo 3la el env
	            map_frame(page_directory,sally_fouad,(uint32)(virtual_address+(khazoo2*PAGE_SIZE)),PERM_PRESENT|PERM_USER|PERM_WRITEABLE);

	        }
	        else
	        {
	            //bnmap ex kimo bs b perms mo5tlpha
	            map_frame(page_directory,sally_fouad,(uint32)(virtual_address+(khazoo2*PAGE_SIZE)),PERM_PRESENT|PERM_USER);

	        }
	    }

	    // bnzwd 3dd el nas elly by7bo mayada
	    mayada1234->references++;

	    // bnrg3 hamada 3shan mytoh4 hhh
	    return hamada;








    //     This function should share the required object in the heap of the current environment
    //    starting from the given virtual_address with the specified permissions of the object: read_only/writable
    //     and return the ShareObjectID
    // RETURN:
    //    a) sharedObjectID (its index in the array) if success
    //    b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists

}

//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//===================
// Free Share Object:
//===================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	short hamada = 69;




	struct Env* kammar12 = curenv; //The calling environment
    uint32*pageDirectory= kammar12->env_page_directory;
    hamada++;
//    cprintf("hamada = %d" , hamada);

	// Steps:
	   if (sharedObjectID >= MAX_SHARES)
	   		return E_SHARED_MEM_NOT_EXISTS;
	    hamada--;


	   //    cprintf("hamada = %d" , hamada);
	    hamada++;
		//	1) Get the shared object from the "shares" array
	    int pagesNum =ROUNDUP(shares[sharedObjectID].size,PAGE_SIZE)/PAGE_SIZE;

	    //    cprintf("hamada = %d" , hamada);
	    hamada--;


	    for(int kammar=0;kammar<pagesNum;kammar++)
	    {


	    	//	2) Unmap it from the current environment "myenv"
	    	unmap_frame( pageDirectory , ((uint32) startVA + ( kammar * PAGE_SIZE )));
	    	uint32 currentVA=(uint32) startVA +( kammar * PAGE_SIZE);
	    	env_page_ws_invalidate( kammar12 , (uint32) currentVA);
	    	//    cprintf("hamada = %d" , hamada);
	        hamada++;





	    	//	3) If one or more table becomes empty, remove it
	    	uint32 *pageTable=NULL;
	    	int isEmpty=1;
	    	get_page_table(pageDirectory, ((uint32)startVA + (kammar * PAGE_SIZE)) , &pageTable);
	    	if(pageTable!=NULL)
	    	{
	    		//    cprintf("hamada = %d" , hamada);
	    	    hamada--;
	    		for(int kammar1=0;kammar1<1024;kammar1++)
				{
					if(pageTable[kammar1]!=0)
					{
						isEmpty=0;
						break;
					}
				}
	    		if(isEmpty==1){
	    			kfree(pageTable);
	    		    hamada++;
	    		//    cprintf("hamada = %d" , hamada);
	    		    }

	    	}
	    }


		//	4) Update references
	    shares[sharedObjectID].references--;
	    //    cprintf("hamada = %d" , hamada);

		//	5) If this is the last share, delete the share object (use free_share_object())
	    if(shares[sharedObjectID].references==0)
	    {
	    	shares[sharedObjectID].empty=1;
	    	free_share_object(sharedObjectID);
	    	//    cprintf("hamada = %d" , hamada);
	        hamada--;

	    }


		//	6) Flush the cache "tlbflush()"
	     tlbflush();


	     //    cprintf("hamada should equal 69 = %d" , hamada);

	     return 0;
}
