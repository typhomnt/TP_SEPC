/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
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
	int size_index = 0;
	while(res < size){
		res = 2*res;
		size_index++;
	}
	return size_index;
}

//size must be a power of 2
// Verifie si on peut desallou
static bool
invalid_free(void *ptr,unsigned long size){
	block_list *temp = NULL;
	for(int i = 0 ; i <= BUDDY_MAX_INDEX ; i++){
		temp = TZL[i];
		while(temp != NULL){
			/* on verifie qu'il n'y a pas de debordement sur
			une case deja libre (ie temp) lors du free. 
			Si non, on passe au bloc suivant de meme taille */
			if((unsigned long)ptr + size > (unsigned long)temp)
				return true;
			temp = temp->suivant;
		}
	}
	return false;
}

static void 
add_head(block_list* block, int size_index){
	block_list* temp = TZL[size_index];
	TZL[size_index] = block;
	TZL[size_index]->suivant = temp;
}

static bool
free_buddy(block_list* block, int index,unsigned long size){
	block_list *temp = TZL[index];
	block_list *buddy = (block_list*)((unsigned long)block^size);
	while(temp != NULL){
		if(temp == buddy)
			return true;
		temp = temp->suivant;
	}
	return false;
}

static int
defrag(block_list* block, int index,unsigned long size){
	block_list *temp = TZL[index];
	block_list *buddy = (block_list*)((unsigned long)block^size);
	int block_defrag = 0;
	int buddy_defrag = 0;
	//Case where the block or its buddy is the head
	if(temp == block){
		block_defrag = 1;
		TZL[index] = TZL[index]->suivant;
	}
	if(temp == buddy){
		buddy_defrag = 1;
		TZL[index] = TZL[index]->suivant;
	}
	while(block_defrag == 0 && buddy_defrag == 0){
		//if we have this case it means that we did an error before (mem_free)
		if(temp == NULL)
			return -1;
		//we delete either the block or the buddy or we go to next free block
		if(temp->suivant == block){
			block_defrag = 1;
			temp->suivant = temp->suivant->suivant;
		}
		else if(temp->suivant == buddy){
			buddy_defrag = 1;
			temp->suivant = temp->suivant->suivant;
		}
		else {
			temp = temp->suivant ;
		}
	}
	//we add a free zone of 2*block_size
	if(block < buddy){
		add_head(block, index + 1);
	}
	else {
		add_head(buddy, index + 1);
	}
	return 0;
}
static void*
subdivizion(unsigned long size, unsigned long requested_size, int index){
	// if there is one free zone with the size
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

// must be divisible by 2 or must be a power of 2 
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
	if(size > ALLOC_MEM_SIZE || size < sizeof(void *))
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

	/* Algo :
	- on désalloue la zone mémoire
	- on cherche le buddy avec (adresse xor size)
	- tant qu'il y a un buddy libre (et que size < alloc_mem_size):
		- on fusionne la zone avec son buddy : on enlève le buddy et la zone de leur liste et on ajoute
		le résultat à la liste supérieure
		- on cherche un buddy libre de taille supérieure
	*/
	unsigned long size2 = next_pow_2(size);
	int index = next_pow_2_index(size);
	//if we free a zone which run over a free zone
	if(invalid_free(ptr, size2))
	   return -1;
	block_list *new_block = ptr;
	add_head(new_block, index);
	while(free_buddy(new_block, index, size2) && size2 < ALLOC_MEM_SIZE){
		if(defrag(new_block, index, size2) == -1)
			return -1;
		index++;
		size2 = 2*size2;
	}
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

