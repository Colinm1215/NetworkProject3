#ifndef MAIN_H
#define MAIN_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "helping_structures.h"

#define TTL_EXPIRED 1
#define MAX_SENDQ_EXCEEDED 2
#define NO_ROUTE_TO_HOST 3
#define SENT_OKAY 4

#define GLOBAL_CONFIG 5
#define ROUTER_CONFIG 6
#define HOST_CONFIG 7
#define ROUTER_TO_ROUTER_LINK 8
#define ROUTER_TO_HOST_LINK 9

#define MAX_QUEUE_LEN 10

int create_socket();
int main(int argc, char *argv[]);
int recv_pkt(int sock, char *buffer, int buff_size);
int send_pkt(int sock, char *buffer, int buff_size, int port, unsigned long nextIP);
void generate_ip_header(char *src_addr, char *dst_addr, int length, int id, int ttl, void *start_of_ip_hdr);
void generate_udp_header(int source_port, int destination_port, int length, void *start_of_udp_hdr);
void logger(char *src_overlay_ip, char *dst_overlay_ip, int ip_ident, int status_code, char *next_hop_ip);
void print_pkt(void *pkt);
void read_configuration_file(FILE* fp);
void do_global_config(char *str);
void do_router_config(char *str);
void do_host_config(char *str);
void do_router_link_config(char *str);
void do_host_link_config(char *str);
void* generate_packet(char *body, char *source_addr, char *dest_addr, int source_port, int dest_port, int body_len, int id, int ttl);
char* get_next_hop(char* dest_overlay_ip);


#endif
