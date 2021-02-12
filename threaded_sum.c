#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep function : waits for seconds
#include <time.h>    // for usleep : waits for microseconds
#include "myutils.c"
#include "myutils.h"

#define NTHREADS 2

int sum = 0;

pthread_mutex_t lock;

void* fsum(void * arg) {
  for(int i=0;i<1000;i++) 
  {
    pthread_mutex_lock(&lock);
 	  sum = sum + 1;
    usleep(100);
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  pthread_t tids[NTHREADS];
  pthread_mutex_init(&lock, NULL);

  startTimer(0);
  for(int i=0; i<NTHREADS; i++) {
    printf("Creating thread %d\n", i);
    pthread_create(&tids[i], NULL, fsum, NULL);
  }

  for(int i=0; i<NTHREADS; i++) {
   pthread_join(tids[i], NULL);
  }
  printf("Sum is %d\n", sum);
  printf("Time spent: %ld\n", endTimer(0));
}