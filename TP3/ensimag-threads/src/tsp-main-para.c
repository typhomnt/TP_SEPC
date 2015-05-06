#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>

#include "tsp-types.h"
#include "tsp-job.h"
#include "tsp-genmap.h"
#include "tsp-print.h"
#include "tsp-tsp.h"
#include "tsp-lp.h"
#include "tsp-hkbound.h"


/* macro de mesure de temps, retourne une valeur en nanosecondes */
#define TIME_DIFF(t1, t2) \
  ((t2.tv_sec - t1.tv_sec) * 1000000000ll + (long long int) (t2.tv_nsec - t1.tv_nsec))


/* tableau des distances */
tsp_distance_matrix_t tsp_distance ={};

/** Param�tres **/
pthread_mutex_t mp = PTHREAD_MUTEX_INITIALIZER;
/* nombre de villes */
int nb_towns=10;
/* graine */
long int myseed= 0;
/* nombre de threads */
int nb_threads=1;

/* affichage SVG */
bool affiche_sol= false;
bool affiche_progress=false;
bool quiet=false;


struct param {
  int* path;
  uint64_t vpres;
  int* sol;
  int* sol_len;
  long long int* cuts;
  int hops;
  int len;
  struct tsp_queue *q;
};
typedef struct param param;
static tsp_path_t sol;
static int sol_len;
static long long int cuts = 0;
static tsp_path_t solution;
static  uint64_t vpres=0;
static struct tsp_queue q;
void* compute_sol(void* args){
  bool ok = false;
  pthread_mutex_lock(&mp);
  ok = empty_queue (&q);
  while (!ok) {
     pthread_mutex_unlock(&mp); 
     int hops = 0, len = 0;
    pthread_mutex_lock(&mp);
    get_job (&q, solution, &hops, &len, &vpres);
    pthread_mutex_unlock(&mp);
    // le noeud est moins bon que la solution courante
    if (minimum < INT_MAX
	&& (nb_towns - hops) > 10
	&& ( (lower_bound_using_hk(solution,hops , len, vpres )) >= minimum
	     || (lower_bound_using_lp(solution,hops , len,vpres)) >= minimum)
	)

      continue;

    tsp (hops ,len, vpres, solution,&cuts , sol,&sol_len );
      pthread_mutex_lock(&mp);
      ok = empty_queue (&q);
  }
  pthread_mutex_unlock(&mp);
  return NULL;
}


static void generate_tsp_jobs (struct tsp_queue *q, int hops, int len, uint64_t vpres, tsp_path_t path, long long int *cuts, tsp_path_t sol, int *sol_len, int depth)
{
    if (len >= minimum) {
        (*cuts)++ ;
        return;
    }
    
    if (hops == depth) {
        /* On enregistre du travail � faire plus tard... */
      add_job (q, path, hops, len, vpres);
    } else {
        int me = path [hops - 1];        
        for (int i = 0; i < nb_towns; i++) {
	  if (!present (i, hops, path, vpres)) {
                path[hops] = i;
		vpres |= (1<<i);
                int dist = tsp_distance[me][i];
                generate_tsp_jobs (q, hops + 1, len + dist, vpres, path, cuts, sol, sol_len, depth);
		vpres &= (~(1<<i));
            }
        }
    }
}

static void usage(const char *name) {
  fprintf (stderr, "Usage: %s [-s] <ncities> <seed> <nthreads>\n", name);
  exit (-1);
}

int main (int argc, char **argv)
{
    unsigned long long perf;
    tsp_path_t path;
    struct timespec t1, t2;
    pthread_t th[nb_threads];
    /* lire les arguments */
    int opt;
    while ((opt = getopt(argc, argv, "spq")) != -1) {
      switch (opt) {
      case 's':
	affiche_sol = true;
	break;
      case 'p':
	affiche_progress = true;
	break;
      case 'q':
	quiet = true;
	break;
      default:
	usage(argv[0]);
	break;
      }
    }

    if (optind != argc-3)
      usage(argv[0]);

    nb_towns = atoi(argv[optind]);
    myseed = atol(argv[optind+1]);
    nb_threads = atoi(argv[optind+2]);
    assert(nb_towns > 0);
    assert(nb_threads > 0);
   
    minimum = INT_MAX;
      
    /* generer la carte et la matrice de distance */
    if (! quiet)
      fprintf (stderr, "ncities = %3d\n", nb_towns);
    genmap ();

    init_queue (&q);

    clock_gettime (CLOCK_REALTIME, &t1);

    memset (path, -1, MAX_TOWNS * sizeof (int));
    path[0] = 0;
    vpres=1;

    /* mettre les travaux dans la file d'attente */
    generate_tsp_jobs (&q, 1, 0, vpres, path, &cuts, sol, & sol_len, 3);
    no_more_jobs (&q);
   
    /* calculer chacun des travaux */
    memset (solution, -1, MAX_TOWNS * sizeof (int));
    solution[0] = 0;
    
    for(int i = 0 ; i < nb_threads ; i++){
	/*	struct param * arg = malloc(sizeof(param*));
	arg->hops = 0;
	arg->len = 0;
	arg->path = solution;
	arg->vpres = vpres;
	arg->cuts = &cuts;
	arg->sol_len = &sol_len;
	arg->sol = sol;
	arg->q = &q;*/
	pthread_create(&th[i],NULL,compute_sol,NULL);
    }
    for (int i = 0; i < nb_threads; i++)
         pthread_join(th[i], NULL);
    
    clock_gettime (CLOCK_REALTIME, &t2);

    if (affiche_sol)
      print_solution_svg (sol, sol_len);

    perf = TIME_DIFF (t1,t2);
    printf("<!-- # = %d seed = %ld len = %d threads = %d time = %lld.%03lld ms ( %lld coupures ) -->\n",
	   nb_towns, myseed, sol_len, nb_threads,
	   perf/1000000ll, perf%1000000ll, cuts);

    return 0 ;
}
