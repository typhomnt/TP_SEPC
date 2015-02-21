/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/
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

static unsigned long 
next_pow_2(unsigned long size){
	unsigned long res = 1;
	while(res < size){
		res = 2*res;
	}
	return res;
}
//a factoriser car deux fois la même fonction
static int
next_pow_2_index(unsigned long size){
	unsigned long res = 1;
	int index = 0;
	while(res < size){
		res = 2*res;
		index++;
	}
	return index;
}

static void 
add_head(block_list* block, int size_index){
	block_list* temp = TZL[size_index];
	TZL[size_index] = block;
	TZL[size_index]->suivant = temp;
}

static void*
subdivizion(unsigned long size, unsigned long requested_size, int index){
	if(TZL[index] != NULL){
		if(size == requested_size){
			void *block = TZL[index];
			//the block is no longer free
			TZL[index] = TZL[index]->suivant;
			return block;
		}
		else {
			//we delete TZL[(int)log2(size)] from the free blocks of log2(size)
			TZL[index] = TZL[index]->suivant;
			//we add the buddy first
			add_head((block_list*)((unsigned long)TZL[index]^size/2), index - 1);
			//then the block pointed by TZL[(int)log2(size)]
			add_head(TZL[index], index - 1);
			return subdivizion(size/2,requested_size, index - 1);
		}
		
	}
	else if (size == ALLOC_MEM_SIZE){
		//we couldn't find a free block
		return 0;
	}
	else {
		//we try to find a free block of 2*size in order to subdivize him
		return subdivizion(2*size, requested_size, index + 1);
	}
}
//size must be divisible by 2
static void*
get_block(unsigned long size, int index){
	if(size % 2 != 0)
		return 0;
	void *block = subdivizion(size, size, index);
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
	if(size > ALLOC_MEM_SIZE)
		return 0;
	unsigned long size2 = next_pow_2(size);
	int pow_2_index = next_pow_2_index(size);
        return get_block(size2,pow_2_index);
}

int 
mem_free(void *ptr, unsigned long size)
{
	/* ecrire votre code ici */
	/*attention on ne peut pas désalouer une zone mémoire de taille < sizeof(void*) sinon
	 on ne pourrai pas stocker l'addresse de la prochaine case libre mais on peut tout de même faire:
	si size < sizeof(void*) chercher si le buddy de cette case est libre ; si c'est le cas on fusionne les deux case
	si la nouvelle taille de cette zone est suffisant */
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

