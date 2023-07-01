#ifndef __COMMON_H__
#define __COMMON_H__

#include <stddef.h>
#include <stdint.h>

#define MAX_CONNECTIONS 32
#define DIMBUF 1551



struct chat_packet {
  uint32_t len;
  char message[DIMBUF];
};
struct abonari {
    char topic[50];
    char *unsenttopic;
    int SF;
};

struct client {
    char ID[20];
    int nr_topics;
    int verificare;
    struct sockaddr_in client_addr;
    struct abonari *topic_struct;
};

int send_all(int sockfd, void *buffer, size_t len) {

  struct chat_packet sent_packet;
  sent_packet.len = len;
  strcpy(sent_packet.message, buffer);
  size_t bytes_sent = 0;
  size_t nr_bytes = 0;
  size_t bytes_remaining = sizeof(struct chat_packet);
  
      while(bytes_remaining) {
          nr_bytes = send(sockfd,&sent_packet+bytes_sent,bytes_remaining,0);
          if(nr_bytes == 0)
                return 0;
          bytes_sent += nr_bytes;
          bytes_remaining -= nr_bytes;
      }
    return bytes_sent;
}
int recv_all(int sockfd, void *buffer, size_t len) {

  size_t bytes_received = 0;
  size_t bytes_remaining = len;
  size_t recv_bytes;
  char *buff = buffer;
      while(bytes_remaining) {
          recv_bytes = recv(sockfd,buff+bytes_received,bytes_remaining,0);
          if(recv_bytes == 0)
                return 0;
		  bytes_received += recv_bytes;
		  bytes_remaining -= recv_bytes;
      }
   return bytes_received;
}



#endif
