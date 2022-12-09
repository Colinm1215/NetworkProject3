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
#include <time.h>
#include <netinet/ether.h>

#define TTL_EXPIRED 1
#define MAX_SENDQ_EXCEEDED 2
#define NO_ROUTE_TO_HOST 3
#define SENT_OKAY 4

int my_port = 0;
char *my_addr = "";
int host_flag = 0;
int router_flag = 0;


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
  server.sin_port = htons(my_port);
  inet_aton(my_addr, &server.sin_addr);
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

int send_pkt(int sock, char *buffer, int buff_size, int port, unsigned long nextIP)
{
  struct sockaddr_in to;
  int tolen, n;

  tolen = sizeof(struct sockaddr_in);

  // Okay, we must populate the to structure.
  bzero(&to, sizeof(to));
  to.sin_family = AF_INET;
  to.sin_port = htons(port);
  to.sin_addr.s_addr = nextIP;

  // We can now send to this destination:
  n = sendto(sock, buffer, buff_size, 0,
             (struct sockaddr *)&to, tolen);

  return n;
}

void generate_ip_header(char *src_addr, char *dst_addr, int length, int id, int ttl, void *start_of_ip_hdr)
{
  ((struct ip *)start_of_ip_hdr)->ip_hl = 5;
  ((struct ip *)start_of_ip_hdr)->ip_len = htons(length);
  ((struct ip *)start_of_ip_hdr)->ip_p = 17;
  ((struct ip *)start_of_ip_hdr)->ip_sum = htons(0);
  ((struct ip *)start_of_ip_hdr)->ip_tos = 0;
  ((struct ip *)start_of_ip_hdr)->ip_ttl = ttl;
  ((struct ip *)start_of_ip_hdr)->ip_id = htons(id);
  ((struct ip *)start_of_ip_hdr)->ip_off = htons(IP_DF);

  struct in_addr src_in_addr;
  int retval;

  memset(&src_in_addr, '\0', sizeof(src_in_addr));
  retval = inet_aton(src_addr, &src_in_addr);

  ((struct ip *)start_of_ip_hdr)->ip_src = src_in_addr;

  struct in_addr dst_in_addr;

  memset(&dst_in_addr, '\0', sizeof(dst_in_addr));
  retval = inet_aton(dst_addr, &dst_in_addr);

  ((struct ip *)start_of_ip_hdr)->ip_dst = dst_in_addr;
}

void generate_udp_header(int source_port, int destination_port, int length, void *start_of_udp_hdr) {
  ((struct udphdr*)start_of_udp_hdr)->uh_sport = htons(source_port);
  ((struct udphdr*)start_of_udp_hdr)->uh_dport = htons(destination_port);
  ((struct udphdr*)start_of_udp_hdr)->uh_ulen = htons(length);
  ((struct udphdr*)start_of_udp_hdr)->uh_sum = htons(sizeof(struct udphdr));
}

void* generate_packet(char *body, char *source_addr, char *dest_addr, int source_port, int dest_port, int body_len, int id, int ttl) {
  int total_pkt_size = sizeof(struct ip) + sizeof(struct udphdr) + sizeof(char)*body_len;
  void *pkt_start = malloc(total_pkt_size);
  void *pkt_working_ptr = pkt_start;

  generate_ip_header(source_addr, dest_addr, total_pkt_size, id, ttl, pkt_working_ptr);

  pkt_working_ptr = pkt_working_ptr + sizeof(struct ip);

  generate_udp_header(source_port, dest_port, total_pkt_size - sizeof(struct ip), pkt_working_ptr);

  pkt_working_ptr = pkt_working_ptr + sizeof(struct udphdr);

  memcpy(pkt_working_ptr, (void *)body, sizeof(char) * body_len);

  return pkt_start;
}

void print_pkt(void *pkt)
{
  char *src_ip = malloc(sizeof(struct in_addr));
  char *dst_ip = malloc(sizeof(struct in_addr));
  int ip_hdr_len = ((struct ip *)pkt)->ip_hl;
  int ip_len = ntohs(((struct ip *)pkt)->ip_len);
  int ip_p = ((struct ip *)pkt)->ip_p;
  int ip_sum = ntohs(((struct ip *)pkt)->ip_sum);
  int tos = ((struct ip *)pkt)->ip_tos;
  int ttl = ((struct ip *)pkt)->ip_ttl;
  int id = ntohs(((struct ip *)pkt)->ip_id);
  int frag = ntohs(((struct ip *)pkt)->ip_off);
  strcpy(src_ip, inet_ntoa(((struct ip *)pkt)->ip_src));
  strcpy(dst_ip, inet_ntoa(((struct ip *)pkt)->ip_dst));
  pkt = pkt + sizeof(struct ip);
  int sp = ntohs(((struct udphdr *)pkt)->uh_sport);
  int dp = ntohs(((struct udphdr *)pkt)->uh_dport);
  int len = ntohs(((struct udphdr *)pkt)->uh_ulen);
  int sum = ntohs(((struct udphdr *)pkt)->uh_sum);
  pkt = pkt + sizeof(struct udphdr);
  char *body = malloc(sizeof(char) * (len - sizeof(struct udphdr)));
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

  printf("body : %s\n", (char *)pkt);
}


void logger(char *src_overlay_ip, char *dst_overlay_ip, int ip_ident, int status_code, char *next_hop_ip)
{

  FILE *f;
  // append message to log file
  f = fopen("project3.log", "a");
  if (f == NULL)
  {
    printf("-----Log Error-----\n");
  }

  time_t t = time(NULL);
  // struct tm tm = *localtime(&t);

  switch (status_code)
  {
  case TTL_EXPIRED:
    fprintf(f, "%ld %s %s %d TTL_EXPIRED\n", t, src_overlay_ip, dst_overlay_ip, ip_ident);
    break;
  case MAX_SENDQ_EXCEEDED:
    fprintf(f, "%ld %s %s %d MAX_SENDQ_EXCEEDED\n", t, src_overlay_ip, dst_overlay_ip, ip_ident);
    break;
  case NO_ROUTE_TO_HOST:
    fprintf(f, "%ld %s %s %d NO_ROUTE_TO_HOST\n", t, src_overlay_ip, dst_overlay_ip, ip_ident);
    break;
  case SENT_OKAY:
    fprintf(f, "%ld %s %s %d SENT_OKAY %s\n", t, src_overlay_ip, dst_overlay_ip, ip_ident, next_hop_ip);
    break;

  default:
    fprintf(f, "%ld Error: no status code found.\n", t);
    fclose(f);
    return;
  }

  fclose(f);
}

int main(int argc, char *argv[]) {

  int opterr = 0;
  int opt;

  while ((opt = getopt (argc, argv, "hri:p:")) != -1) {
    switch (opt) {
      case 'h':
        host_flag = 1;
        break;
      case 'r':
        router_flag = 1;
        break;
      case 'i':
        if (optarg[0] != "-") {
          my_addr = (char *) malloc(sizeof(char)*strlen(optarg));
          strcpy(my_addr, optarg);
        } else {
          printf("IP argument is not a valid address\n");
          printf("Usage : %s [-h OR -r] [-i Address] [-p Port]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n-i indicates the IP address, and -p indicates the port", argv[0]);
          exit(1);
        }
        break;
      case 'p':
        if (optarg[0] != "-") {
          my_port = atoi(optarg);
        } else {
          printf("Port argument is not a valid port\n");
          printf("Usage : %s [-h OR -r] [-i Address] [-p Port]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n-i indicates the IP address, and -p indicates the port", argv[0]);
          exit(1);
        }
        break;
      default:
        printf("Usage : %s [-h OR -r] [-i Address] [-p Port]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n-i indicates the IP address, and -p indicates the port", argv[0]);
        exit(1);
      }
  }

  if (router_flag == 1 && host_flag == 1) {
    printf("Usage : %s [-h OR -r] [-i Address] [-p Port]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n-i indicates the IP address, and -p indicates the port", argv[0]);
    exit(1);
  } else if (router_flag == 0 && host_flag == 0) {
    printf("Usage : %s [-h OR -r] [-i Address] [-p Port]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n-i indicates the IP address, and -p indicates the port", argv[0]);
    exit(1);
  }

  int sock = create_socket();
  
  if (router_flag == 1) {
    // make forwarding table
  }

  if  (host_flag == 1) {
    // get file and send
  }

  while (router_flag == 1) {
    char *buffer = (char *) malloc(sizeof(char)*1000);
    printf("Looking\n");
    int returnVal = recv_pkt(sock, buffer, 1000);
    printf("Recieved %d : %s\n", returnVal, buffer);
    void *ptr = ((void *) buffer) + sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct udphdr);
    print_pkt(ptr);
    
    if (returnVal == -1) {
      perror("failed");
      continue;
    }

    ((struct iphdr*)ptr)->ttl--;
  }

int count = 0;
  while (host_flag == 1 && count < 1) {
    char *message = "According to all known laws of aviation, there is no way a bee should be able to fly. It's wings are too small to get its fat little body off the ground";
    void *pkt = generate_packet(message, my_addr, "192.168.153.1", 1024, 1025, strlen(message), 0, 10);
    printf("Sending : %s", pkt);
    struct in_addr antelope;
char *some_addr;
//print_pkt(pkt);
// 192.168.153.255 ./main -h -i 130.215.169.168 -p 1024
//inet_aton("127.0.0.1", &antelope);
int val = inet_pton(AF_INET, "192.168.153.1", &antelope);
    printf("Sending : %s", pkt);
    send_pkt(sock, pkt, sizeof(struct ip) + sizeof(struct udphdr) + strlen(message), 1025, antelope.s_addr); 
    count++;
  }

}
