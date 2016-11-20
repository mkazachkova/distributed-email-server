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
  list->num_nodes--;
  return;
}


void insert(List *list, void *data, int (*compare)(void *, void *)) {
  Node *n = malloc(sizeof(Node));
  n->data = malloc(list->node_size);
  n->prev= NULL;
  n->next = NULL; 

  memcpy(n->data, data, list->node_size);

  //Accounts for inserting into empty list
  if (list->num_nodes == 0) {
    list->head = n;
    list->tail = n;
    list->num_nodes++;
    return;
  }

  Node *temp = list->head;
  while (temp != NULL) {
    if ((*compare)(temp->data, data) < 0) {
      //do nothing
    } else if ((*compare)(temp->data, data) >= 0) {
      if (list->num_nodes == 1) {
        temp->prev = n;
        n->next = temp;
        list->head = n;
        list->num_nodes++;
        return;
      } else {
        //general case
        if (temp->prev == NULL) {
          n->next = temp;
          temp->prev = n;
          list->head = n;
          list->num_nodes++;
          return;
        } else {
          n->prev = temp->prev;
          temp->prev->next = n;
          n->next = temp;
          temp->prev = n;
          list->num_nodes++;
          return;
        }
      }
     
    }
    temp = temp->next;
   
  }

  //reached end of list but haven't inserted yet
  //case in which node is greater than all values in list
  list->tail->next = n; 
  n->prev = list->tail; 
  list->tail = n;       
  list->num_nodes++;
  return;
}


Node* find(List *list, void* data, int (*compare)(void *, void *)) {
  Node *temp = list->head;
  while (temp != NULL) {
    if ((*compare)(temp->data, data) == 0) {
      return temp;
    }
    temp = temp->next;
  }
  //node not found
  return NULL;
}


// Accessor methods
void print_list(List *list, void (*fptr)(void *)) {
  Node* temp = list->head;

  while (temp != NULL) {
    (*fptr)(temp->data);
    temp = temp->next;
  }
  printf("\n");
  return;
}


void print_list_backwards(List *list, void (*fptr)(void *)) {
  Node* temp = list->tail;

  while (temp != NULL) {
    (*fptr)(temp->data);
    temp = temp->prev;    
  }
  printf("\n");
  return;
}
