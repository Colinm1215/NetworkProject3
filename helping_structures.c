#include "helping_structures.h"

struct Trie_Node *create_trie_node() {
    struct Trie_Node *node = (struct Trie_Node *)malloc(sizeof(struct Trie_Node));
    node->destination = 0;
    node->children[0] = NULL;
    node->children[1] = NULL;
    return node;
}

void insert_ip(struct Trie_Node *root, char *overlay_ip_addr, char *real_ip_addr) {
    struct Trie_Node *node = root;
    for (int i = 0; i < strlen(overlay_ip_addr); i++) {
        int bit = overlay_ip_addr[i] - '0';
        if (node->children[bit] == NULL) {
            node->children[bit] = create_trie_node();
        }
        node = node->children[bit];
    }
    node->destination = 1;
    node->real_addr = real_ip_addr;
}

void search_ip(struct Trie_Node *root, char *overlay_ip_addr, char *ip_return) {
    struct Trie_Node *node = root;

    for (int i = 0; i < strlen(overlay_ip_addr); i++) {
        int bit = overlay_ip_addr[i] - '0';
        if (node->destination == 1) {
            strcpy(ip_return, node->real_addr);
        }
        
        if (node->children[bit] == NULL) {
            break;
        }

        node = node->children[bit];
    }

    if (node->destination == 1) {
        strcpy(ip_return, node->real_addr);
    }
}