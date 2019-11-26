#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 3000
#define BUFSIZE 1024

int main(int argc, char const *argv[])
{
  fd_set master;
  fd_set read_fds;
  int fdmax, i;
  int sockfd = 0;
  struct sockaddr_in server_addr, client_addr;
  int opt = 1;
  char buffer[1024] = {0};
  int login[10] = {0};

  // Create server
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  memset(server_addr.sin_zero, '\0', sizeof(server_addr.sin_zero));
  fdmax = sockfd;
  
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, 5) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("\nServer Waiting for client on port %d\n", PORT);
  fflush(stdout);
  FD_SET(sockfd, &master);
  
  while (1) {
    read_fds = master;

    // Block until for new event
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    // Connection accept
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
	if (i == sockfd) {
	  socklen_t addrlen;
	  int newsockfd, nbytes_recvd;
	  char recv_name_buf[BUFSIZE], recv_pass_buf[BUFSIZE];

	  addrlen = sizeof(struct sockaddr_in);
	  if ((newsockfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen)) == -1) {
	    perror("accept");
	    exit(EXIT_FAILURE);
	  }
	  else {
	    FD_SET(newsockfd, &master);
	    
	    if (newsockfd > fdmax) {
	      fdmax = newsockfd;
	    }
	    
	    printf("new connection from %s on port %d \n",inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	    // Log in
	    /*if ((recv(newsockfd, recv_name_buf, BUFSIZE, 0) <= 0)) {
	      perror("recv");
	    }
	    else {
	      printf("%s", recv_name_buf);
	      // Check account
	      if ((strcmp(recv_name_buf, "hai") == 0) && (strcmp(recv_pass_buf, "123") == 0)) {
		printf("%s log in\n", recv_name_buf);
	      }
	      else {
		close(newsockfd);
		FD_CLR(newsockfd, &master);
	      }
	      }*/
	  }
	}
	else {
	  // Send and receive
	  int nbytes_recvd, j;
	  char recv_buf[BUFSIZE], buf[BUFSIZE];
	
	  if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) {
	    if (nbytes_recvd == 0) {
	      printf("socket %d hung up\n", i);
	    }
	    else {
	      perror("recv");
	    }
	    
	    close(i);
	    FD_CLR(i, &master);
	  }
	  else {
	    printf("%s\n", recv_buf);

	    // Send all
	    for(j = 0; j <= fdmax; j++){
	      if (FD_ISSET(j, &master)){
		if (j != sockfd && j != i) {
		  if (send(j, recv_buf, nbytes_recvd, 0) == -1) {
		    perror("send");
		  }
		}
	      }
	    }
	  }
	}
      }
    }
  }

  return 0;
}
