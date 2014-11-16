/*
 * Name: Jiayang Li
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <ucontext.h>
#include <errno.h>
#include "threadsalive.h"
#include "list.h"

#define STACKSIZE 128000

static list_t *ready_queue;
static ucontext_t *main_context;
static ucontext_t *current_context;

/* ***************************** 
     stage 1 library functions
   ***************************** */

void ta_libinit(void) {
    //initialize ready_queue
    ready_queue = (list_t *) malloc(sizeof(list_t));
    assert(ready_queue);
    list_init(ready_queue);

    //initialize main_context and current_context
    main_context = (ucontext_t *) malloc(sizeof(ucontext_t));
    assert(main_context);

    current_context = main_context;
}
 
void ta_create(void (*func)(void *), void *arg) {
    //create a new ucontext
    ucontext_t *nctx = (ucontext_t *) malloc(sizeof(ucontext_t));
    assert(nctx);   //check new context

    unsigned char *stack = (unsigned char *) malloc(STACKSIZE);
    assert(stack);  //check stack

    if (getcontext(nctx) == -1){
        fprintf(stderr, "getcontext failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    (nctx->uc_stack).ss_sp = stack;
    (nctx->uc_stack).ss_size = STACKSIZE;
    nctx->uc_link = main_context; //link back to main

    makecontext(nctx, (void (*)(void))func, 1, arg);
    list_add(ready_queue, nctx);
}

void ta_yield(void) {
    //if there is no thread left, let this last thread run to completion
    //printf("queue size: %d\n", list_size(ready_queue));
    if (list_size(ready_queue) > 0){
        ucontext_t *to_run = list_remove(ready_queue);
        list_add(ready_queue, current_context);
        current_context = to_run;
        if (swapcontext((ready_queue->tail)->ctx, to_run) == -1){
            fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }


}

int ta_waitall(void) {
    while (list_size(ready_queue) > 0){
        current_context = list_remove(ready_queue);
        if (swapcontext(main_context, current_context) == -1){
            fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //thread has completed, free the allocated memory
        free((current_context->uc_stack).ss_sp);
        free(current_context);
    }
    free(ready_queue);
    free(main_context);
    return 0;
}


/* ***************************** 
     stage 2 library functions
   ***************************** */

void ta_sem_init(tasem_t *sem, int value) {
}

void ta_sem_destroy(tasem_t *sem) {
}

void ta_sem_post(tasem_t *sem) {
}

void ta_sem_wait(tasem_t *sem) {
}

void ta_lock_init(talock_t *mutex) {
}

void ta_lock_destroy(talock_t *mutex) {
}

void ta_lock(talock_t *mutex) {
}

void ta_unlock(talock_t *mutex) {
}


/* ***************************** 
     stage 3 library functions
   ***************************** */

void ta_cond_init(tacond_t *cond) {
}

void ta_cond_destroy(tacond_t *cond) {
}

void ta_wait(talock_t *mutex, tacond_t *cond) {
}

void ta_signal(tacond_t *cond) {
}

