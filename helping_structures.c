#include "helping_structures.h"

struct Trie_Node *create_trie_node() {
    struct Trie_Node *node = (struct Trie_Node *)malloc(sizeof(struct Trie_Node));
    node->destination = 0;
    node->children[0] = NULL;
    node->children[1] = NULL;
    return node;
}

void insert_ip(struct Trie_Node *root, char *overlay_ip_addr, char *real_ip_addr) {
    char *trun_overlay_ip = (char *)malloc(sizeof(char)*strlen(overlay_ip_addr));
    char *endstr = overlay_ip_addr + (strlen(overlay_ip_addr) - 2);
    if (strcmp(endstr, ".0") == 0) {
        strncpy(trun_overlay_ip, overlay_ip_addr, strlen(overlay_ip_addr)-2);
        printf("%s\n", trun_overlay_ip);
    } else {
        strncpy(trun_overlay_ip, overlay_ip_addr, strlen(overlay_ip_addr));
    }


    struct Trie_Node *node = root;
    for (int i = 0; i < strlen(trun_overlay_ip); i++) {
        int n = trun_overlay_ip[i];
        int a[10],i;
        for(i=0;n>0;i++) {    
            a[i]=n%2;    
            n=n/2;    
        }
        for(i=i-1;i>=0;i--) {
            int bit = a[i];
            if (node->children[bit] == NULL) {
                node->children[bit] = create_trie_node();
            }
            node = node->children[bit];
            node->destination = 0;
            node->real_addr = NULL;
        }
    }
    node->destination = 1;
    node->real_addr = real_ip_addr;
}

char* search_ip(struct Trie_Node *root, char *overlay_ip_addr) {
    struct Trie_Node *node = root;
    char *ip_return;
    char *last_found_ip;
    for (int i = 0; i < strlen(overlay_ip_addr); i++) {
        int n = overlay_ip_addr[i];
        int a[10],i;
        for(i=0;n>0;i++) {    
            a[i]=n%2;    
            n=n/2;    
        }
        for(i=i-1;i>=0;i--) {
            int bit = a[i];
        
            if (node->children[bit] == NULL || node->destination == 1) {
                break;
            }

            node = node->children[bit];
        }
        
        if (node->destination == 1) {
            last_found_ip = (char *) malloc(sizeof(char)*strlen(node->real_addr));
            strncpy(last_found_ip, node->real_addr, strlen(node->real_addr));
        } else {
            while (node->children[0] != NULL || node->children[1] != NULL) {
                if (node->children[0] != NULL && node->children[1] == NULL) {
                    node = node->children[0];
                } else if (node->children[1] != NULL && node->children[0] == NULL) {
                    node = node->children[1];
                } else if (node->children[0] != NULL && node->children[1] != NULL) {
                    int len1 = 0;
                    struct Trie_Node *node1 = node;
                    while (node1->children[0] != NULL) {
                        node1 = node1->children[0];
                        len1++;
                    }
                    int len2 = 0;
                    struct Trie_Node *node2 = node;
                    while (node2->children[1] != NULL) {
                        node2 = node2->children[1];
                        len2++;
                    }
                    if (len1 >= len2) node = node->children[0];
                    else node = node->children[1];
                }
            }
        }
    }

    return last_found_ip;
}

void get_IP_from_overlay(struct Host_Node *hosts, char *ip, char *ret) {
    int f = 0;

    while (hosts != NULL) {
        if (strcmp(ip, hosts->r_addr) == 0) {
            f = 1;
            strcpy(ret, hosts->o_addr);
        }
        hosts = hosts->next;
    }

    if (f == 0) {
        strcpy(ret, "-");
    }
}