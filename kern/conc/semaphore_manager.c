#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/environment_definitions.h>

#include "semaphore_manager.h"
#include <kern/proc/user_environment.h>
#include <kern/cpu/sched.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===============================
// [1] Create "semaphores" array:
//===============================
//Dynamically allocate the "semaphores" array
//initialize the "semaphores" array by 0's and empty = 1
void create_semaphores_array(uint32 numOfSemaphores)
{
#if USE_KHEAP
	MAX_SEMAPHORES = numOfSemaphores ;
	semaphores = (struct Semaphore*) kmalloc(numOfSemaphores*sizeof(struct Semaphore));
	if (semaphores == NULL)
	{
		panic("Kernel runs out of memory\nCan't create the array of semaphores.");
	}
#endif
	for (int i = 0; i < MAX_SEMAPHORES; ++i)
	{
		memset(&(semaphores[i]), 0, sizeof(struct Semaphore));
		semaphores[i].empty = 1;
		LIST_INIT(&(semaphores[i].env_queue));
	}

}


//========================
// [2] Allocate Semaphore:
//========================
//Allocates a new (empty) semaphore object from the "semaphores" array
//Return:
//	a) if succeed:
//		1. allocatedSemaphore (pointer to struct Semaphore) passed by reference
//		2. SempahoreObjectID (its index in the array) as a return parameter
//	b) E_NO_SEMAPHORE if the the array of semaphores is full (i.e. reaches "MAX_SEMAPHORES")
int allocate_semaphore_object(struct Semaphore **allocatedObject)
{
	int32 semaphoreObjectID = -1 ;
	for (int i = 0; i < MAX_SEMAPHORES; ++i)
	{
		if (semaphores[i].empty)
		{
			semaphoreObjectID = i;
			break;
		}
	}

	if (semaphoreObjectID == -1)
	{
		//try to double the size of the "semaphores" array
		#if USE_KHEAP
		{
			semaphores = (struct Semaphore*) krealloc(semaphores, 2*MAX_SEMAPHORES);
			if (semaphores == NULL)
			{
				*allocatedObject = NULL;
				return E_NO_SEMAPHORE;
			}
			else
			{
				semaphoreObjectID = MAX_SEMAPHORES;
				MAX_SEMAPHORES *= 2;
			}
		}
		#else
		{
			panic("Attempt to dynamically allocate space inside kernel while kheap is disabled .. ");
			*allocatedObject = NULL;
			return E_NO_SEMAPHORE;
		}
		#endif
	}

	*allocatedObject = &(semaphores[semaphoreObjectID]);
	semaphores[semaphoreObjectID].empty = 0;

	return semaphoreObjectID;
}

//======================
// [3] Get Semaphore ID:
//======================
//Search for the given semaphore object in the "semaphores" array
//Return:
//	a) if found: SemaphoreObjectID (index of the semaphore object in the array)
//	b) else: E_SEMAPHORE_NOT_EXISTS
int get_semaphore_object_ID(int32 ownerID, char* name)
{
	int i=0;
	for(; i < MAX_SEMAPHORES; ++i)
	{
		if (semaphores[i].empty)
			continue;

		if(semaphores[i].ownerID == ownerID && strcmp(name, semaphores[i].name)==0)
		{
			return i;
		}
	}
	return E_SEMAPHORE_NOT_EXISTS;
}

//====================
// [4] Free Semaphore:
//====================
//delete the semaphore with the given ID from the "semaphores" array
//Return:
//	a) 0 if succeed
//	b) E_SEMAPHORE_NOT_EXISTS if the semaphore is not exists
int free_semaphore_object(uint32 semaphoreObjectID)
{
	if (semaphoreObjectID >= MAX_SEMAPHORES)
		return E_SEMAPHORE_NOT_EXISTS;

	memset(&(semaphores[semaphoreObjectID]), 0, sizeof(struct Semaphore));
	semaphores[semaphoreObjectID].empty = 1;
	LIST_INIT(&(semaphores[semaphoreObjectID].env_queue));

	return 0;
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//======================
// [1] Create Semaphore:
//======================
int createSemaphore(int32 ownerEnvID, char* semaphoreName, uint32 initialValue)
{

struct Semaphore* semaphoreOBJ;

for(int i =0 ; i < MAX_SEMAPHORES; ++i)
{
	if (semaphores[i].empty)continue;
	if(semaphores[i].ownerID == ownerEnvID && strcmp(semaphoreName, semaphores[i].name)==0)
	{return E_SEMAPHORE_EXISTS;
	}
}
	int res = allocate_semaphore_object(&semaphoreOBJ);

	if ( 0x0000 <= res){

		semaphoreOBJ->ownerID = ownerEnvID;
		semaphoreOBJ->value = initialValue;
		//======================
		strcpy(semaphoreOBJ->name,semaphoreName);

		return res;
	}
	else return E_NO_SEMAPHORE;
	//======================

return 0;
}

//============
// [2] Wait():
//============
void waitSemaphore(int32 ownerEnvID, char* semaphoreName)
{
	struct Env* ENENENENENEN = curenv; //The calling environment
	int waddy = get_semaphore_object_ID(ownerEnvID,semaphoreName);
	/*
	 * opjvfo
	 * public
	 * static*/
	semaphores[waddy].value--;
	if (semaphores[waddy].value<0){
		 enqueue(&semaphores[waddy].env_queue, ENENENENENEN);
		 /*
		 	 * opjvfo
		 	 * public
		 	 * static
		 	 * foreach 7amada
		 	 * smsm*/
		 ENENENENENEN->env_status =ENV_BLOCKED;
		 curenv =NULL;
	   }
	/*
		 */
	 fos_scheduler();
}
//==============
// [3] Signal():
//==============
void signalSemaphore(int ownerEnvID, char* semaphoreName)
{



//current env
struct Env* kammar;

//semaphore index
int samy= get_semaphore_object_ID(ownerEnvID,semaphoreName);


//updating value
semaphores[samy].value-=-1;

            if (!(semaphores[samy].value>0)){

                /* this is code logic*/
        kammar= dequeue(&semaphores[samy].env_queue);
        /* je suis criossant, inserting env to ready queue */
        sched_insert_ready(kammar);



        //updating status to ready



        kammar->env_status=ENV_READY;
      }
}

