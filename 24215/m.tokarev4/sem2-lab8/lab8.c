#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define num_steps 1000000000

typedef struct {
    int id;
    int cntthreads;
    double res;
}info;

struct timeval tv1, tv2, dtv;
struct timezone tz;
void time_start() { gettimeofday(&tv1, &tz); }
long time_stop()
{
    gettimeofday(&tv2, &tz);
    dtv.tv_sec = tv2.tv_sec - tv1.tv_sec;
    dtv.tv_usec = tv2.tv_usec - tv1.tv_usec;
    if (dtv.tv_usec < 0) { dtv.tv_sec--; dtv.tv_usec += 1000000; }
    return dtv.tv_sec * 1000 + dtv.tv_usec / 1000;

}

void* scoringpartofpi(void* arg) {
    info* args = (info*)arg;
    int id = args->id;
    int cntthreads = args->cntthreads;

    double pi = 0;
    double fl = 0;

    for (int i = id; i < num_steps; i += cntthreads) {
        long long znam = 1 + 2 * i;
        if (i % 2 == 0) {
            fl = 1;
        }
        else {
            fl = -1;
        }
        pi += fl / znam;
    }
    args->res = pi;
    return NULL;
}



int main(int argc, char** argv) {

    if (argc < 2) {
        printf("No argums");
        perror("NO_ARGUMS");
        return 1;
    };

    int cntthreads = atoi(argv[1]);
    cntthreads = 1;
    for (int i = 0; i < 8; i++) {

        for (int i = 0; i < 52; i++) {
            if (i == 2) {
                time_start();
            }
            pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * cntthreads);
            info* args = (info*)malloc(sizeof(info) * cntthreads);

            for (int i = 0; i < cntthreads; i++) {
                args[i].id = i;
                args[i].cntthreads = cntthreads;
                pthread_create(&threads[i], NULL, scoringpartofpi, (void*)&args[i]);
            }

            double pi = 0;

            for (int j = 0; j < cntthreads; j++) {
                pthread_join(threads[j], NULL);
                pi += args[j].res;
            }

            pi = pi * 4.0;
            //printf("pi done - %.15g \n", pi);
            free(args);
            free(threads);
        }
        printf("Time on %d threads %ld\n", cntthreads, time_stop()/50);
        cntthreads *= 2;
    }
    return (EXIT_SUCCESS);
}

