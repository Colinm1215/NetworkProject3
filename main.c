#include "main.h"

int my_port = 5000;
int my_id = -1;
char *my_addr;
char *my_overlay_addr;
char *my_router_ip;
int host_flag = 0;
int router_flag = 0;
int queue_length = 0;
int ttl_value = 0;

struct Queue {
  char pkts[MAX_QUEUE_LEN][1000];
  int len;
  int head;
  int tail;
};

struct Router_Node *all_routers = NULL;
struct Host_Node *all_hosts = NULL;
struct Trie_Node *prefix_matching = NULL;


int create_socket()
{
  int sock;
  struct sockaddr_in server;

  sock = socket(AF_INET, SOCK_DGRAM, 0);

  if (sock < 0)
    perror("Error creating CS3516 socket");

  bzero(&server, sizeof(server));
  server.sin_family = AF_INET;
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

// int drop_pkt(int sock, )

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

  strncpy((char *)pkt_working_ptr, body, strlen(body));

  return pkt_start;
}

void print_pkt(void *pkt)
{
  char *src_ip = (char *)malloc(sizeof(struct in_addr));
  char *dst_ip = (char *)malloc(sizeof(struct in_addr));
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
  int s = sizeof(char) * ((len) - sizeof(struct udphdr));
  char body[s+1];
  body[s] = '\0';
  strncpy(body, (char *)pkt, s);
  
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

  printf("body : %s\n", body);
  free(src_ip);
  free(dst_ip);
}

// pkt payload max 1000

// DROP-TAIL QUEUING
int droptail_enqueue(struct Queue *queue, char *pkt){
  
  // drop packet when droptail queue is already full
  if(queue->len == MAX_QUEUE_LEN){
    return 0; // queue already full
  }
  strcpy(queue->pkts[queue->tail], pkt);
  queue->tail = (queue->tail + 1) % MAX_QUEUE_LEN;
  queue->len++;
  return 1; // enqueue successful
}

int droptail_dequeue(struct Queue *queue, char *pkt){

  pkt = (char *) malloc(sizeof(char)*1000);
  // when droptail queue is empty
  if(queue->len == 0){
    return 0; // empty queue
  }
  strcpy(pkt, queue->pkts[queue->head]);
  // queue->pkts[queue->head] = '\0';
  memset(queue->pkts[queue->head], '\0', 1000);
  queue->head = (queue->head + 1) % MAX_QUEUE_LEN;
  queue->len--;
  return 1; // dequeue successful
}



void logger(char *src_overlay_ip, char *dst_overlay_ip, int ip_ident, int status_code, char *next_hop_ip)
{

  FILE *f;
  // append message to log file
  f = fopen("ROUTER_control.txt", "a");
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

void do_global_config(char *str) {
  char delim[] = " ";

	char *ptr = strtok(str, delim);
  int pos = 0;

	while (ptr != NULL) {
    switch (pos) {
      case 0:
        queue_length = atoi(ptr);
        break;
      case 1:
        ttl_value = atoi(ptr);
        break;
    }
    pos++;
		ptr = strtok(NULL, delim);
  }
}


void do_router_config(char *str) {
  char delim[] = " ";
  struct Router_Node *head = all_routers;

	char *ptr = strtok(str, delim);
  int pos = 0;

	while (ptr != NULL) {
    switch (pos) {
      case 0:
        if (atoi(ptr) == my_id) {
          while (head != NULL) {
            head = head->next;
          }
          head = (struct Router_Node*) (malloc(sizeof(struct Router_Node)));
          head->id = atoi(ptr);
          router_flag = 1;
          break;
        }
        else return;
      case 1:
        if (head->id == my_id) {
          my_addr = (char *)malloc(sizeof(char)*strlen(ptr));
          strncpy(my_addr, ptr, strlen(ptr)-2);
        }
        head->delay_with_router = 0;
        head->addr = (char *)malloc(sizeof(char)*strlen(ptr));
        strncpy(head->addr, ptr, strlen(ptr)-2);
        break;
    }
    pos++;
		ptr = strtok(NULL, delim);
  }
}

void do_host_config(char *str) {
  char delim[] = " ";
  struct Host_Node *head;
  if (all_hosts == NULL) {
    all_hosts = (struct Host_Node*) (malloc(sizeof(struct Host_Node)));
    all_hosts->next = NULL;
    head = all_hosts;
  } else {
    head = all_hosts;
    while (head->next != NULL) {
      head = head->next;
    }
    head->next = (struct Host_Node*) (malloc(sizeof(struct Host_Node)));
    head = head->next;
    head->next = NULL;
  }

	char *ptr = strtok(str, delim);
  int pos = 0;

	while (ptr != NULL) {
    char buffer[strlen(ptr)];
    strcpy(buffer, ptr);
    if (pos == 2) {
      buffer[strcspn(buffer, "\n")-1] = 0;
    }
    switch (pos) {
      case 0:
        head->id = atoi(ptr);
        if (atoi(ptr) == my_id) {
          host_flag = 1;
        }
        break;
      case 1:
        if (head->id == my_id) {
          my_addr = (char *)malloc(sizeof(char)*strlen(buffer));
          strncpy(my_addr, buffer, strlen(buffer));
        }
        head->r_addr = (char *)malloc(sizeof(char)*strlen(buffer));
        strcpy(head->r_addr, buffer);
        break;
      case 2:
          if (head->id == my_id) {
          my_overlay_addr = (char *)malloc(sizeof(char)*strlen(buffer));
          strncpy(my_overlay_addr, buffer, strlen(buffer));
          }
        head->o_addr = (char *)malloc(sizeof(char)*strlen(buffer));
        strcpy(head->o_addr, buffer);
        break;
    }
    pos++;
		ptr = strtok(NULL, delim);
  }
}

void do_router_link_config(char *str) {
  if (router_flag == 0) return 0;
  char delim[] = " ";
  struct Router_Node *head = all_routers;


	char *ptr = strtok(str, delim);
  int pos = 0;
  int first_id = -1;
  int second_id = -1;
  int first_delay = 0;
  int second_delay = 0;

	while (ptr != NULL) {
      switch (pos) {
        case 0:
          first_id = atoi(ptr);
          break;
        case 1:
          first_delay = atoi(ptr);
          break;
        case 2:
          second_id = atoi(ptr);
          break;
        case 3:
          second_delay = atoi(ptr);
          break;
      }
      pos++;
		  ptr = strtok(NULL, delim);
  }

  if (first_id == my_id) {
    while (head != NULL) {
      if (head->id == second_id) break;
      else head = head->next;
    }
    if (head != NULL) {
      head->delay_with_router = second_delay;
    }
  } else if (second_id == my_id) {
    while (head != NULL) {
      if (head->id == first_id) break;
      else head = head->next;
    }
    if (head != NULL) {
      head->delay_with_router = first_delay;
    }
  }
}

void do_host_link_config(char *str) {
  char delim[] = " ";
  struct Host_Node *head_hosts = all_hosts;
  struct Router_Node *head_routers = all_routers;

	char *ptr = strtok(str, delim);
  int pos = 0;
  int first_id = -1;
  int second_id = -1;
  char *prefix_addr;
  int first_delay = 0;
  int second_delay = 0;

	while (ptr != NULL) {
      switch (pos) {
        case 0:
          first_id = atoi(ptr);
          break;
        case 1:
          first_delay = atoi(ptr);
          break;
        case 2:
          char *str = strtok(ptr, "/");
          prefix_addr = (char *)malloc(sizeof(char)*strlen(str));
          strcpy(prefix_addr, str);
          break;
        case 3:
          second_id = atoi(ptr);
          break;
        case 4:
          second_delay = atoi(ptr);
          break;
      }
      pos++;
		  ptr = strtok(NULL, delim);
  }

  if (router_flag == 1 && first_id == my_id) {
    while (head_hosts != NULL) {
      if (head_hosts->id == second_id) {
        insert_ip(prefix_matching, head_hosts->o_addr, head_hosts->r_addr);
        break;
      }
    }
  } else if (host_flag == 1 && second_id == my_id) {
    while (head_routers != NULL) {
      if (head_routers->id == first_id) {
        my_router_ip = (char *)(malloc(sizeof(char)*strlen(head_routers->addr)));
        strcpy(my_router_ip, head_routers->addr);
        break;
      }
    }
  } else {
    char *rtr_addr;
    while (head_routers != NULL) {
      if (head_routers->id == first_id) {
        rtr_addr = (char *)(malloc(sizeof(char)*strlen(head_routers->addr)));
        strcpy(rtr_addr, head_routers->addr);
        break;
      }
    }

    insert_ip(prefix_matching, prefix_addr, rtr_addr);
  }
}

void read_configuration_file(FILE* fp) {
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  while ((read = getline(&line, &len, fp)) != -1) {
    int flag = ((int)line[0]-'0')+5;
    strcpy(line, line + 2);

    switch (flag) {
        case GLOBAL_CONFIG:
          do_global_config(line);
          break;
        case ROUTER_CONFIG:
          do_router_config(line);
          break;
        case HOST_CONFIG:
          do_host_config(line);
          break;
        case ROUTER_TO_ROUTER_LINK:
          do_router_link_config(line);
          break;
        case ROUTER_TO_HOST_LINK:
          do_host_link_config(line);
          break;
      }
  }

  printf("Queue Value - %d\nTTL Val - %d\n", queue_length, ttl_value);
  printf("Router_Flag : %d\nHost_Flag : %d\n", router_flag, host_flag);
  printf("Address : %s:%d\n", my_addr, my_port);
  if (host_flag == 1) {
  printf("Overlay Address : %s\n", my_overlay_addr);
  }

  fclose(fp);
  if (line) free(line);
}

int main(int argc, char *argv[]) {
/*
    struct in_addr tip;
    int val = inet_pton(AF_INET, "6.5.4.1", &tip);
    char *src_ip = (char *)malloc(sizeof(struct in_addr));
    tip.s_addr = (tip.s_addr & SUBNET_MASK(24));
  strcpy(src_ip, inet_ntoa(tip));
  */



  int opterr = 0;
  int opt;
  int n = 10;
  char arg_err[1000];   // Use an array which is large enough 
  snprintf(arg_err, sizeof(arg_err), "Usage : %s [-i ID] [-p Port] [-f FILENAME]\n-i indicates the machine ID\n-p indicates the port\n-f is the filename of the configuration file\n", argv[0]);

  while ((opt = getopt (argc, argv, "i:p:f:")) != -1) {
    switch (opt) {
      case 'f':
        if (optarg[0] != "-") {
          FILE * fp;
          char *file_name = (char *) malloc(sizeof(char)*strlen(optarg));
          strcpy(file_name, optarg);
          fp = fopen(file_name, "r");
          if (fp == NULL) {
            printf("Not a valid filename\n"); 
            printf("%s", arg_err);
            exit(1);
          }
          read_configuration_file(fp);
        } else {
          printf("Not a valid filename\n");
          printf("%s", arg_err);
          exit(1);
        }
        break;
      case 'i':
        if (optarg[0] != "-") {
          my_id = atoi(optarg);
        } else {
          printf("Invalid ID Number\n");
          printf("%s", arg_err);
          exit(1);
        }
        break;
      case 'p':
        if (optarg[0] != "-") {
          my_port = atoi(optarg);
        } else {
          printf("Port argument is not a valid port\n");
          printf("%s", arg_err);
          exit(1);
        }
        break;
      default:
          printf("%s", arg_err);
        exit(1);
      }
  }

  if (router_flag == 1 && host_flag == 1) {
    printf("Host/Router Identification Failed!\nSet to both Host and Router\n");
    exit(1);
  } else if (router_flag == 0 && host_flag == 0) {
    printf("Host/Router Identification Failed!\nSet to neither Host nor Router\n");
    exit(1);
  }

  int sock = create_socket();

  if  (host_flag == 1) {
    // get file and send
  }

  while (router_flag == 1) {
    printf("Reading\n");
    char *buffer = (char *) malloc(sizeof(char)*1000);
    int returnVal = recv_pkt(sock, buffer, 1000);
    printf("Read %d bytes\n", returnVal);

    if (returnVal == -1) {
      perror("failed");
      continue;
    }
    
    void *ptr = ((void *) buffer);
    print_pkt(ptr);

    ((struct iphdr*)ptr)->ttl--;

    // drops packet (ignores) and logs when TTL value is zero
    if(((struct iphdr*)ptr)->ttl == 0){
      // logger(src_overlay_ip, dst_overlay_ip, ip_ident, TTL_EXPIRED, NULL);
      free(buffer);
      continue;
    }

    // TODO: send packet along to next router

    free(buffer);
  }

  int count = 0;
  while (host_flag == 1 && count < 1) {
    char *message = "According to all known laws of aviation, there is no way a bee should be able to fly. It's wings are too small to get its fat little body off the ground";
    void *pkt = generate_packet(message, my_overlay_addr, all_hosts->o_addr, 8135, 8134, strlen(message), 0, ttl_value);
    struct in_addr dest_ip;
    char *some_addr;
    int val = inet_pton(AF_INET, my_router_ip, &dest_ip);
    int returnVal = send_pkt(sock, (char *)pkt, sizeof(struct ip) + sizeof(struct udphdr) + strlen(message), 8134, dest_ip.s_addr);
    free(pkt);
    count++;
  }

}
