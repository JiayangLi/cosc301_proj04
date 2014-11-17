/*
 * Name: Jiayang Li
 * 
 * A queue implementation for maintaining thread schedule
 * Adapted from previous lab
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ucontext.h>
#include "list.h"

void list_init(list_t *queue){
 	queue->head = NULL;
 	queue->tail = NULL;
 	queue->size = 0;
}

//add to tail
void list_add(list_t *queue, ucontext_t *nctx){
	struct __list_node *addition = (struct __list_node *) malloc(sizeof(struct __list_node));

	if (!addition){
		fprintf(stderr, "No memory while attempting to create a new list node!\n");
        abort();
	}

	addition->ctx = nctx;
	addition->next = NULL;

	//if the queue was empty
	if (queue->head == NULL && queue->tail == NULL){
		queue->head = addition;
		queue->tail = addition;
	} else {	//if the queue wasn't empty
		(queue->tail)->next = addition;
		queue->tail = addition;
	}

	queue->size++;
}

//remove from head
ucontext_t *list_remove(list_t *queue){
	//if the queue was already empty
	if (queue->head == NULL && queue->tail == NULL){
		return NULL;
	}

	ucontext_t *rv = (queue->head)->ctx;
	struct __list_node *to_free = queue->head;
	queue->head = (queue->head)->next;

	//update tail if necessary
	if (queue->head == NULL){
		queue->tail = NULL;
	}

	free(to_free);
	queue->size--;
	return rv;
}

int list_size(list_t *queue){
	return queue->size;
}

void list_clear(list_t *queue){
	//remember to free everything in queue
	//e.g. node, ucontext, stack
	while (queue->head != NULL){
		struct __list_node *to_free = queue->head;
		queue->head = (queue->head)->next;
		free(((to_free->ctx)->uc_stack).ss_sp);
		free(to_free->ctx);
		free(to_free);
	}
	free(queue);
}

