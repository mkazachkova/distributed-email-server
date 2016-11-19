/*
Sarah Sukardi
Mariya Kazachkova
Distributed Systems: Final Project
December 9, 2016
*/

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

// Node struct 
typedef struct node {
  void* data;
  struct node *next;
  struct node *prev;
} Node;

typedef struct {
  Node *head;
  Node *tail;
  int num_nodes;
  int node_size;
} List;

// List manipulation methods
void create_list(List *list, size_t data_size, void (*fptr)(void *));
void add_to_end(List *list, void *data);
void remove_from_beginning(List *list);
void remove_from_end(List *list);
void insert(List *list, void *data, void (*fptr)(void *));
Node* find(List *list, void* data, void (*fptr)(void *));


// Accessor methods
// (last element is a fxn ptr)
void print_list(List *list, void (*fptr)(void *));
void print_list_backwards(List *list, void (*fptr)(void *));
