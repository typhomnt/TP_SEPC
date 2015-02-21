/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;


/* ecrire votre code ici */
struct block_list {
	struct block_list *suivant;
};

typedef struct block_list block_list;

block_list *TZL[BUDDY_MAX_INDEX + 1];

double 
log2(double n)  
{    
    return log(n)/log(2);  
}


static void 
add_head(block_list* block, int size_index){
	block_list* temp = TZL[size_index];
	TZL[size_index] = block;
	TZL[size_index]->suivant = temp;
}

static void*
subdivizion(unsigned long size, unsigned long requested_size){
	if(TZL[(int)log2(size)] != NULL){
		if(size == requested_size){
			void *block = TZL[(int)log2(size)];
			//the block is no longer free
			TZL[(int)log2(size)] = TZL[(int)log2(size)]->suivant;
			return block;
		}
		else {
			//we delete TZL[(int)log2(size)] from the free blocks of log2(size)
			TZL[(int)log2(size)] = TZL[(int)log2(size)]->suivant;
			//we add the buddy first
			add_head((block_list*)((unsigned long)TZL[(int)log2(size)]^size/2), (int)log2(size/2));
			//then the block pointed by TZL[(int)log2(size)]
			add_head(TZL[(int)log2(size)], (int)log2(size/2));
			return subdivizion(size/2,requested_size);
		}
		
	}
	else if (size == ALLOC_MEM_SIZE){
		//we couldn't find a free block
		return 0;
	}
	else {
		//we try to find a free block of 2*size in order to subdivize him
		return subdivizion(2*size, requested_size);
	}
}
//size must be divisible by 2
static void*
get_block(unsigned long size){
	if(size % 2 != 0)
		return 0;
	void *block = subdivizion(size, size);
	if(block == 0)
		return 0;
	return block;
}

int 
mem_init()
{
  if (! zone_memoire)
    zone_memoire = (void *) malloc(ALLOC_MEM_SIZE);
  if (zone_memoire == 0)
    {
      perror("mem_init:");
      return -1;
    }

  /* ecrire votre code ici */
  TZL[BUDDY_MAX_INDEX] = zone_memoire;
  TZL[BUDDY_MAX_INDEX]->suivant = NULL;
  for(int i = 0 ; i < BUDDY_MAX_INDEX ; i++)
	  TZL[i] = NULL;
  return 0;
}

void *
mem_alloc(unsigned long size)
{
  /*  ecrire votre code ici */
	//the size must be divisible by 2
	unsigned long size2 = pow(2,ceil(log2(size)));
        return get_block(size2);
}

int 
mem_free(void *ptr, unsigned long size)
{
	/* ecrire votre code ici */
	return 0;
}


int
mem_destroy()
{
  /* ecrire votre code ici */

	for(int i = 0 ; i <= BUDDY_MAX_INDEX ; i++)
		TZL[i] = NULL;
	free(zone_memoire);
	zone_memoire = 0;
	return 0;
}

