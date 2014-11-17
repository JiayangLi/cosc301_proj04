#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include "threadsalive.h"

static tasem_t sem;

void test(void *n)
{
	ta_sem_wait(&sem);
    int *i = (int *) n;
    fprintf(stderr, "begin t1: %d\n", *i);
    //ta_yield();
    *i += 7;
    fprintf(stderr, "end t1: %d\n", *i);
    //ta_sem_post(&sem);
}


int main(int argc, char **argv)
{
    printf("Tester for stage 1.  Uses all four stage 1 library functions.\n");

    ta_libinit();
    ta_sem_init(&sem, 1);

    int i = 0;
    for (i = 0; i < 5; i++) {
    	
        ta_create(test, (void *)&i);
    }

    printf("waitall() returns: %d\n", ta_waitall());
    
    ta_sem_destroy(&sem);
    return 0;
}