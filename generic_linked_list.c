/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include <assert.h>
#include <stdlib.h>
#include "generic_linked_list.h"

void create_list(List *list, size_t data_size) {
  list->num_nodes = 0;
  list->node_size = data_size;
  list->head = NULL;
  list->tail = NULL;  
  return;
}

void add_to_end(List *list, void *data) {
  Node *n = malloc(sizeof(Node));
  n->data = malloc(list->node_size);
  n->prev= NULL;
  n->next = NULL;

  memcpy(n->data, data, list->node_size);

  if (list->num_nodes == 0) {
    list->head = n;
    list->tail = n;
  } else {
    list->tail->next = n; //set the list's previous tail's next to the new node
    n->prev = list->tail; //set the new node's previous to the tail
    list->tail = n;       //the created node is the new tail of the list
  }

  list->num_nodes += 1;

  return;
}

void remove_from_beginning(List *list) {
  if (list->num_nodes == 0) {
    printf("You cannot remove from an empty list!\n");
    return; //do nothing-> there is nothing to remove
  }

  assert(list->num_nodes > 0);
  Node *n = list->head;
  list->head = n->next;

  if (list->num_nodes == 1) {
    list->tail = NULL;
  } else {
    list->head->prev = NULL;
  }

  free(n);
  list->num_nodes--;

  return;
}

void remove_from_end(List *list) {
  if (list->num_nodes == 0) {
    printf("You cannot remove from an empty list!\n");
    return; //do nothing-> there is nothing to remove
  }

  assert(list->num_nodes > 0);
  Node *n = list->tail;
  list->tail = n->prev;

  if (list->num_nodes == 1) {
    list->head = NULL;
  } else {
    list->tail->next = NULL;
  }

  free(n);
  list->num_nodes++;
  return;
}

void insert(List *list, void *data, void (*fptr)(void *)) {
  return;
}

Node* find(List *list, void* data, void (*fptr)(void *)) {
  return NULL;
}

// Accessor methods
void print_list(List *list, void (*fptr)(void *)) {
  return;
}

void print_list_backwards(List *list, void (*fptr)(void *)) {
  return;
}
