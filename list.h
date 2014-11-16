/*
 * Name: Jiayang Li
 */

#ifndef __LIST_H__
#define __LIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ucontext.h>

struct __list_node {
    ucontext_t *ctx;
    struct __list_node *next;
};

typedef struct {
    struct __list_node *head;
    struct __list_node *tail;
    int size;
} list_t;

void list_init(list_t *);
void list_add(list_t *, ucontext_t *);
ucontext_t *list_remove(list_t *);
int list_size(list_t *);

#endif
