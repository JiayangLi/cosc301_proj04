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
static ucontext_t *current_context; //keeps track of the currently running thread
static int num_blocked = 0;  //indicates the number of blocked threads

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

    num_blocked = 0;
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

        // main_context gets here in two possible cases:
        // 1)current_context finishes
        // 2)current_context is the last ready thread and it blocks
        // in both cases waitall is about to return and needs to free current_context
        free((current_context->uc_stack).ss_sp);
        free(current_context);
    }

    list_clear(ready_queue);
    free(main_context);
    return num_blocked == 0 ? 0 : -1;
}


/* ***************************** 
     stage 2 library functions
   ***************************** */

void ta_sem_init(tasem_t *sem, int value) {
    sem->value = value;
    sem->blocked_queue = (list_t *) malloc(sizeof(list_t));
    list_init(sem->blocked_queue);
}

void ta_sem_destroy(tasem_t *sem) {
    list_clear(sem->blocked_queue);
}

void ta_sem_post(tasem_t *sem) {
    sem->value++;

    //put a blocked thread (if any) to the ready queue
    if (list_size(sem->blocked_queue) > 0){
        ucontext_t *to_ready = list_remove(sem->blocked_queue);
        num_blocked--;  //removed one blocked thread
        list_add(ready_queue, to_ready);
    }
}

void ta_sem_wait(tasem_t *sem) {
    //block until the value > 0

    while (sem->value <= 0){
        if (list_size(ready_queue) <= 0){   //no more ready thread, switch back to main
            //don't add to blocked_queue, let waitall() free this thread
            num_blocked++;    //added one blocked thread
            if (swapcontext(current_context, main_context) == -1){
                fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        } else {    //switch to the next ready thread
            ucontext_t *to_run = list_remove(ready_queue);
            list_add(sem->blocked_queue, current_context);
            num_blocked++;    //added one blocked thread
            current_context = to_run;
            if (swapcontext(((sem->blocked_queue)->tail)->ctx, to_run) == -1){
                fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }

    sem->value--;
}

void ta_lock_init(talock_t *mutex) {
    mutex->lock = (tasem_t *) malloc(sizeof(tasem_t));
    ta_sem_init(mutex->lock, 1);
}

void ta_lock_destroy(talock_t *mutex) {
    ta_sem_destroy(mutex->lock);
    free(mutex->lock);
}

void ta_lock(talock_t *mutex) {
    ta_sem_wait(mutex->lock);
}

void ta_unlock(talock_t *mutex) {
    ta_sem_post(mutex->lock);
}


/* ***************************** 
     stage 3 library functions
   ***************************** */

void ta_cond_init(tacond_t *cond) {
    cond->blocked_queue = (list_t *) malloc(sizeof(list_t));
    list_init(cond->blocked_queue);
}

void ta_cond_destroy(tacond_t *cond) {
    list_clear(cond->blocked_queue);
}

void ta_wait(talock_t *mutex, tacond_t *cond) {
    ta_unlock(mutex);

    //put current_context to the blocked_queue associated with cond
    if (list_size(ready_queue) <= 0){   //no more ready thread, switch back to main
        //don't add to blocked_queue, let waitall() free this thread
        num_blocked++;    //added one blocked thread
        if (swapcontext(current_context, main_context) == -1){
            fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {    //switch to the next ready thread
        ucontext_t *to_run = list_remove(ready_queue);
        list_add(cond->blocked_queue, current_context);
        num_blocked++;    //added one blocked thread
        current_context = to_run;
        if (swapcontext(((cond->blocked_queue)->tail)->ctx, to_run) == -1){
            fprintf(stderr, "swapcontext failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        ta_lock(mutex);
    }
}

void ta_signal(tacond_t *cond) {
    //put a blocked (if any) to the ready_queue
    if (list_size(cond->blocked_queue) > 0){
        ucontext_t *to_ready = list_remove(cond->blocked_queue);
        num_blocked--;  //removed one blocked thread
        list_add(ready_queue, to_ready);
    }
}

