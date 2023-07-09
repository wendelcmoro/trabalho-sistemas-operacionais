// Wendel Caio Moro GRR20182641
// João Victor Frans Pondaco Winandy GRR20182617

#include "queue.h"
#include <stdio.h>

void queue_append (queue_t **queue, queue_t *elem){
    if (!queue) return;
    //printf ("1\n");

    if (elem->next != NULL || elem->prev != NULL) return;

    //printf ("2\n");
    queue_t *first;
    if (!*queue) { 
        first = elem;
        first->next = first;
        first->prev = first;
        *queue = (queue_t *) first;
        return;
    }

    first = *queue;        
    queue_t *current = first;
    while (current->next != first) current = current->next;
    
    current->next = elem;
    elem->next = first;
    elem->prev = current;
    first->prev = elem;
    
    return;
}

queue_t *queue_remove (queue_t **queue, queue_t *elem){
    if (!queue) return NULL;

    queue_t *first = *queue;
    queue_t *current = first;

    if (current == elem) {
        if (first->next == first) *queue = NULL;
        else *queue = first->next;
    } else {
        current = current->next;
        while (current != first && current != elem) {
            current = current->next;
        }
    }

    if (current != elem) return NULL;

    current->next->prev = current->prev;
    current->prev->next = current->next;
    current->next = NULL;
    current->prev = NULL;
    return current;
}

int queue_size (queue_t *queue){
    if (!queue) return (0);

    int tam = 1;
    queue_t *first = queue;
    queue_t *current = queue;

    while (current->next != first && current->next != NULL){
        tam++;
        current = current->next;
    }

    return tam;
}

void queue_print (char *name, queue_t *queue, void print_elem (void*) ){
    printf ("%s", name);
    printf (": [");

    if (queue) {
        queue_t *first = queue;
        queue_t *current = queue;
        while (current->next != first){ 
            print_elem (current);
            current = current->next;
            printf (" ");
        }
        print_elem (current);
    }

    printf ("]\n");
}
