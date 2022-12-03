#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

int recv(int sock, char *buffer, int buff_size)
{
  struct sockaddr_in from;
  int fromlen, n;
  fromlen = sizeof(struct sockaddr_in);
  n = recvfrom(sock, buffer, buff_size, 0,
               (struct sockaddr *)&from, &fromlen);

  return n;
}

int send(int sock, char *buffer, int buff_size, unsigned long nextIP)
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

void* generate_udp_header(int source_port, int destination_port, int length, void *start_of_udp_hdr) {
  struct udphdr *hdr = (struct udphdr*) malloc(sizeof(struct udphdr));

  hdr->uh_sport = source_port;
  hdr->uh_dport = destination_port;
  hdr->uh_ulen = length;
  hdr->uh_sum = sizeof(char)*8;

  start_of_udp_hdr = hdr;

  return start_of_udp_hdr + sizeof(struct udphdr);
}

int main(int argc, char *argv[]) {
  int host_flag = 0;
  int router_flag = 0;

  char *arg_err = "Usage : %s [-h OR -r]\nWherein -h indicates a host configuration and a -r indicates a router configuration\nOnly one may be used\n";

  int opterr = 0;
  int opt;

  if (argc > 2) {
    printf("%s", arg_err);
    exit(1);
  }

  while ((opt = getopt (argc, argv, "hr:")) != -1) {
    switch (opt) {
      case 'h':
        host_flag = 1;
        break;
      case 'r':
        router_flag = 1;
        break;
      default:
        printf("%s", arg_err);
        exit(1);
      }
  }

  if (router_flag == 1 && host_flag == 1) {
    printf("%s", arg_err);
    exit(1);
  }

  if (router_flag == 1) {

  } else if (host_flag == 1) {
    
  } else {
    printf("%s", arg_err);
    exit(1);
  }

}
