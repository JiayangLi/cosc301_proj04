#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>

#include "threadsalive.h"
#include "list.h"

tacond_t condv;
talock_t mutex;
int value = 0;

void thread(void *v){
	ta_lock(&mutex);
	fprintf(stderr, "I am just a random thread!\n");
	ta_wait(&mutex, &condv);
	fprintf(stderr, "me again!! But quit...\n");
}

int main(){

	ta_libinit();

    ta_lock_init(&mutex);
    ta_cond_init(&condv);

    for (int i = 0; i < 5; i++){
    	ta_create(thread, NULL);
    }

    int rv = ta_waitall();
    printf("waitall() returns: %d\n", rv);

    ta_lock_destroy(&mutex);
    ta_cond_destroy(&condv);
    return 0;
}