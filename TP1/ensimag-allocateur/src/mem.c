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

struct block_list {
    struct block_list *suivant;
};

typedef struct block_list block_list;

block_list *TZL[BUDDY_MAX_INDEX + 1];

//retourne la première puissance de deux après size 
static unsigned long 
next_pow_2(unsigned long size){
    unsigned long res = 1;
    while(res < size){
	res = 2*res;
    }
    return res;
}

//retourne l'indice de puissance de la première puissance de deux après size
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

//size doit être une puissance de deux
/*que l' on peut effectivement libérer la zone mémoire et 
qu'il n'y a pas de débordement sur une case libre*/
static bool
invalid_free(void *ptr,unsigned long size){
    block_list *temp;
    //on verifie en plus que ptr est bien une addresse valide (puissance de 2)
    //(en effet tout ce qui est alloué est une puissance de 2, on ne peut pas libérer un champ qui n'en n'est pas une)
    if((ptr - zone_memoire) % 2 != 0)
	return true;
    for(int i = 0 ; i <= BUDDY_MAX_INDEX ; i++){
	temp = TZL[i];
	while(temp != NULL){
	    /* on verifie qu'il n'y a pas de debordement sur
	       une case deja libre (ie temp) lors du free. 
	       Si non, on passe au bloc suivant de meme taille */
	    if(ptr <= (void*)temp && (unsigned long)ptr + size > (unsigned long)temp)
		return true;
	    temp = temp->suivant;
	}
    }
    return false;
}
//Ajoute block en tête de liste de TZL[size_index]
static void 
add_head(block_list* block, int size_index){
    block_list* temp = TZL[size_index];
    TZL[size_index] = block;
    TZL[size_index]->suivant = temp;
}

//Renvoie true si le buddy du block libéré (qui est en tête de liste à ce moment là) est libre
//Renvoie false sinon
static bool
free_buddy(int index,unsigned long size){
    block_list *temp = TZL[index];
    block_list *buddy = (block_list*)(((unsigned long)(((void*)temp - zone_memoire))^size) + (unsigned long)zone_memoire);
    while(temp != NULL){
	if(temp == buddy)
	    return true;
	temp = temp->suivant;
    }
    return false;
}

//fusionne deux buddy pour en faire une case libre de taille size*2
//cette fonction n'est pas récursive, mem_alloc fait un appel itératif à cette fonction
static int
defrag(int index,unsigned long size){
    block_list *temp = TZL[index];
    block_list *buddy = (block_list*)(((unsigned long)(((void*)temp - zone_memoire))^size) + (unsigned long)zone_memoire);
    block_list *block = TZL[index];
    int buddy_defrag = 0;
    while(buddy_defrag == 0){
        //Si on tombe dans ce cas ci c'est que l'on a fait une erreur interne avant (mem_free)
	if(temp == NULL)
	    return -1;
	//On détruit le buddy et le block concerné ou on va au block suivant
	else if(temp->suivant == buddy){
	    buddy_defrag = 1;
	    temp->suivant = temp->suivant->suivant;
	    TZL[index] = TZL[index]->suivant;
	}
	else {
	    temp = temp->suivant ;
	}
    }
    //On ajoute une zone libre de taille 2*size
    if(block < buddy){
	add_head(block, index + 1);
    }
    else {
	add_head(buddy, index + 1);
    }
    return 0;
}

/*Subdivize un block de taille size s'il en existe un et on appel la fonction avec size/2 pour faire une subdivision récursive.
Si le block  est de taille requise il n'y a pas de subdivision et on retourne ce dernier.
Si il n'y a pas de block libre de taille size on appelle la fonction avec size*2.
Si size == ALLOC_MEM_SIZE et TZL[BUDDY_MAX_INDEX] = NULL alors on a pas trouvé de case libre, l'allocation est impossible*/
static void*
subdivizion(unsigned long size, unsigned long requested_size, int index){
    // S'il y a une zone libre de taille size 
    if(TZL[index] != NULL){
	if(size == requested_size){
	    void *block = TZL[index];
	    //le block est alloué et n'est plus libre
	    TZL[index] = TZL[index]->suivant;
	    return block;
	}
	else {
	    //On libère TZL[index] des zones libres de taille size
	    block_list *block = TZL[index];
	    TZL[index] = TZL[index]->suivant;
	    //On ajoute d'abord le buddy...
	    block_list *buddy = (void*)block + size/2;
	    add_head(buddy, index - 1);
	    //Puis block dans les zones libre de taille size/2
	    add_head(block, index - 1);
	    return subdivizion(size/2,requested_size, index - 1);
	}
		
    }
    else if (size == ALLOC_MEM_SIZE){
	//Il n'y a pas de zone libre disponoble pour requested size
	return 0;
    }
    else {
	//On essaye de trouver une zone libre de taille 2*size dans le but de le subdiviser
	return subdivizion(2*size, requested_size, index + 1);
    }
}

// récupère le block alloué ou une erreur (0)
// size doit être une puissance de 2
static void*
get_block(unsigned long size, int index){
    if(size % 2 != 0)
	return 0;
    void *block = subdivizion(size, size, index);
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
    TZL[BUDDY_MAX_INDEX] = zone_memoire;
    TZL[BUDDY_MAX_INDEX]->suivant = NULL;
    for(int i = 0 ; i < BUDDY_MAX_INDEX ; i++)
	TZL[i] = NULL;
    return 0;
}

void *
mem_alloc(unsigned long size)
{
    //on vérifie que size est une valeur valide
    if(size > ALLOC_MEM_SIZE || size <= 0)
	return 0;
    /*si size est plus petite que la taille minimale d'une zone libre (sizeof(block_list*)) 
      on l'affecte à cette taille minimale */
    if(size < sizeof(block_list *)){
	size = sizeof(block_list *);
    }
	
    unsigned long size2 = next_pow_2(size);
    int pow_2_index = next_pow_2_index(size);
    return get_block(size2,pow_2_index);
}

int 
mem_free(void *ptr, unsigned long size)
{
    /* Algo :
       - on désalloue la zone mémoire
       - on cherche le buddy avec (adresse xor size)
       - tant qu'il y a un buddy libre (et que size < alloc_mem_size):
       - on fusionne la zone avec son buddy : on enlève le buddy et la zone de leur liste et on ajoute
       le résultat à la liste supérieure
       - on cherche un buddy libre de taille supérieure
    */
    if(ptr < zone_memoire || ptr > zone_memoire + ALLOC_MEM_SIZE)
	return -1;
    if(size < sizeof(block_list *)){
	size = sizeof(block_list *);
    }
    unsigned long size2 = next_pow_2(size);
    int index = next_pow_2_index(size);
    //on vérifie si on fait une libération valide
    if(invalid_free(ptr, size2))
	return -1;
    block_list *new_block = ptr;
    add_head(new_block, index);
    while(free_buddy(index, size2) && size2 < ALLOC_MEM_SIZE){
	if(defrag(index, size2) == -1)
	    return -1;
	index++;
	size2 = 2*size2;
    }
    return 0;
}


int
mem_destroy()
{
    for(int i = 0 ; i <= BUDDY_MAX_INDEX ; i++)
	TZL[i] = NULL;
    free(zone_memoire);
    zone_memoire = 0;
    return 0;
}

