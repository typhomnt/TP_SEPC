#ifndef _TSP_JOB_H
#define _TSP_JOB_H
/* gestion des files de jobs */

#include <stdint.h>

/* Structure pour la t�te de file */
struct tsp_queue {
   struct tsp_cell *first;
   struct tsp_cell *last;
   int end;
   int nbmax;
   int nb;
};

/* Initialise la file [q]. */
extern void init_queue (struct tsp_queue *q);

/* Ajoute un job � la file [q]: chemin [p], [hops] villes parcourues, longueur parcourue [len]. */
extern void add_job (struct tsp_queue *q, tsp_path_t p, int hops, int len, uint64_t vpres) ;

/* Enl�ve un job de la file [q], le stocke dans [p], [hops] et [len]. Peut retourner 0 si la file
 * est temporairement vide. */
extern int get_job (struct tsp_queue *q, tsp_path_t p, int *hops, int *len, uint64_t *vpres) ;

/* Enregistre qu'il n'y aura plus de jobs ajout�s � la file. */
extern void no_more_jobs (struct tsp_queue *q) ;

/* Retourne 1 si la file est vide (i.e. no_more_jobs() a �t� appel�, et tous
 * les jobs ont �t� enlev�s), 0 sinon. */
extern int empty_queue (struct tsp_queue *q);

#endif /* _TSP_JOBS */
