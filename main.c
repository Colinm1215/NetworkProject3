#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <stdlib.h>

// Set the following port to a unique number:
#define MYPORT 5950

int create_socket()
{
  int sock;
  struct sockaddr_in server;

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0)
    perror("Error creating CS3516 socket");

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(MYPORT);
  if (bind(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    perror("Unable to bind CS3516 socket");

  // Socket is now bound:
  return sock;
}

int recv_pkt(int sock, char *buffer, int buff_size)
{
  struct sockaddr_in from;
  int fromlen, n;
  fromlen = sizeof(struct sockaddr_in);
  n = recvfrom(sock, buffer, buff_size, 0,
               (struct sockaddr *)&from, &fromlen);

  return n;
}

int send_pkt(int sock, char *buffer, int buff_size, unsigned long nextIP)
{
  struct sockaddr_in to;
  int tolen, n;

  tolen = sizeof(struct sockaddr_in);

  // Okay, we must populate the to structure.
  bzero(&to, sizeof(to));
  to.sin_family = AF_INET;
  to.sin_port = htons(MYPORT);
  to.sin_addr.s_addr = nextIP;

  // We can now send to this destination:
  n = sendto(sock, buffer, buff_size, 0,
             (struct sockaddr *)&to, tolen);

  return n;
}

void generate_ip_header(char *src_addr, char *dst_addr, int length, int id, int ttl, void *start_of_ip_hdr) {
  ((struct ip*)start_of_ip_hdr)->ip_hl = 5;
  ((struct ip*)start_of_ip_hdr)->ip_len = htons(length);
  ((struct ip*)start_of_ip_hdr)->ip_p = 17;
  ((struct ip*)start_of_ip_hdr)->ip_sum = htons(0);
  ((struct ip*)start_of_ip_hdr)->ip_tos = 0;
  ((struct ip*)start_of_ip_hdr)->ip_ttl = ttl;
  ((struct ip*)start_of_ip_hdr)->ip_id = htons(id);
  ((struct ip*)start_of_ip_hdr)->ip_off = htons(IP_DF);

  struct in_addr src_in_addr;
  int retval;
   
  memset(&src_in_addr, '\0', sizeof(src_in_addr));
  retval = inet_aton(src_addr, &src_in_addr);

  ((struct ip*)start_of_ip_hdr)->ip_src = src_in_addr;

  struct in_addr dst_in_addr;
   
  memset(&dst_in_addr, '\0', sizeof(dst_in_addr));
  retval = inet_aton(dst_addr, &dst_in_addr);

  ((struct ip*)start_of_ip_hdr)->ip_dst = dst_in_addr;

}

void generate_udp_header(int source_port, int destination_port, int length, void *start_of_udp_hdr) {

  ((struct udphdr*)start_of_udp_hdr)->uh_sport = htons(source_port);
  ((struct udphdr*)start_of_udp_hdr)->uh_dport = htons(destination_port);
  ((struct udphdr*)start_of_udp_hdr)->uh_ulen = htons(length);
  ((struct udphdr*)start_of_udp_hdr)->uh_sum = htons(sizeof(struct udphdr));
}

void* generate_packet(char *body, char *source_addr, char *dest_addr, int source_port, int dest_port, int body_len, int id, int ttl) {
  int total_pkt_size = sizeof(struct ip) + sizeof(struct udphdr) + sizeof(char)*body_len;
  printf("Allocating %d bytes of memory to packet from %d ip hdr, %d udp hdr, and %d body len\n", total_pkt_size, sizeof(struct ip), sizeof(struct udphdr), body_len);
  void *pkt_start = malloc(total_pkt_size);
  void *pkt_working_ptr = pkt_start;

  printf("Generating IP hdr with src_addr %s, dst_addr %s, and total size %d\n", source_addr, dest_addr, total_pkt_size);
  generate_ip_header(source_addr, dest_addr, total_pkt_size, id, ttl, pkt_working_ptr);

  pkt_working_ptr = pkt_working_ptr + sizeof(struct ip);

  printf("Generating UDP header with source port %d, destination port %d, and body length %d\n", source_port, dest_port, body_len);
  generate_udp_header(source_port, dest_port, total_pkt_size - sizeof(struct ip), pkt_working_ptr);

  pkt_working_ptr  = pkt_working_ptr + sizeof(struct udphdr);
  
  memcpy(pkt_working_ptr, (void *)body, sizeof(char)*body_len);

  return pkt_start;
}

void print_pkt(void *pkt) {
  char *src_ip = malloc(sizeof(struct in_addr));
  char *dst_ip = malloc(sizeof(struct in_addr));
  int ip_hdr_len = ((struct ip*)pkt)->ip_hl;
  int ip_len = ntohs(((struct ip*)pkt)->ip_len);
  int ip_p = ((struct ip*)pkt)->ip_p;
  int ip_sum = ntohs(((struct ip*)pkt)->ip_sum);
  int tos = ((struct ip*)pkt)->ip_tos;
  int ttl = ((struct ip*)pkt)->ip_ttl;
  int id = ntohs(((struct ip*)pkt)->ip_id);
  int frag = ntohs(((struct ip*)pkt)->ip_off);
  strcpy(src_ip, inet_ntoa(((struct ip*)pkt)->ip_src));
  strcpy(dst_ip, inet_ntoa(((struct ip*)pkt)->ip_dst));
  pkt = pkt + sizeof(struct ip);
  int sp = ntohs(((struct udphdr*) pkt)->uh_sport);
  int dp = ntohs(((struct udphdr*) pkt)->uh_dport);
  int len = ntohs(((struct udphdr*) pkt)->uh_ulen);
  int sum = ntohs(((struct udphdr*) pkt)->uh_sum);
  pkt = pkt + sizeof(struct udphdr);
  char *body = malloc(sizeof(char)*(len-sizeof(struct udphdr)));
  strcpy(body, (char *)pkt);

  printf("ip_hdr_len : %d\n", ip_hdr_len);
  printf("ip_len : %d\n", ip_len);
  printf("ip protocol : %d\n", ip_p);
  printf("ip tos : %d\n", tos);
  printf("ip id : %d\n", id);
  printf("ip sum : %d\n", ip_sum);
  printf("ip ttl : %d\n", ttl);
  printf("ip frag : %d\n", frag);
  printf("src addr : %s\n", src_ip);
  printf("dst addr : %s\n", dst_ip);

  printf("SP : %d\n", sp);
  printf("DP : %d\n", dp);
  printf("len : %d\n", len);
  printf("sum : %d\n", sum);

  printf("body : %s\n", pkt);
}

int main(int argc, char *argv[]) {
  int host_flag = 0;
  int router_flag = 0;

  int opterr = 0;
  int opt;

  if (argc > 2) {
    printf("Usage : %s [-h OR -r]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n", argv[0]);
    exit(1);
  }

  while ((opt = getopt (argc, argv, "hr")) != -1) {
    switch (opt) {
      case 'h':
        host_flag = 1;
        break;
      case 'r':
        router_flag = 1;
        break;
      default:
        printf("Usage : %s [-h OR -r]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n", argv[0]);
        exit(1);
      }
  }

  if (router_flag == 1 && host_flag == 1) {
    printf("Usage : %s [-h OR -r]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n", argv[0]);
    exit(1);
  }

  if (router_flag == 1) {

  } else if (host_flag == 1) {
    
  } else {
    printf("Usage : %s [-h OR -r]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n", argv[0]);
    exit(1);
  }

  char body[] = "This is my body";
  char *src_addr = "68.178.157.132";
  char *dst_addr = "255.255.11.135";
  void *pkt = generate_packet(body, src_addr, dst_addr, 1000, 2000, sizeof(body), 10, 100);

  print_pkt(pkt);
}
