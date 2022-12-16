#ifndef HELPING_STRUCTURES_H
#define HELPING_STRUCTURES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUBNET_MASK(n) ((1u << (n)) - 1)
#define NETWORK_MASK(n) (~SUBNET_MASK(n))

struct General_Node {
  char *data;
  int pos;
  struct General_Node *next;
};

struct Trie_Node {
  int destination;
  char *real_addr;
  struct Trie_Node *children[2];
};

struct Trie_Node *create_trie_node();
void insert_ip(struct Trie_Node *root, char *overlay_ip_addr, char *real_ip_addr);
void search_ip(struct Trie_Node *root, char *overlay_ip_addr, char *ip_return);

#endif
