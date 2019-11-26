#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define PORT 3000
#define BUFSIZE 1024
			
int main(int argc, char const *argv[])
{
  int sockfd, fdmax, i;
  struct sockaddr_in server_addr;
  fd_set master;
  fd_set read_fds;
  char name[256] = "";
  char password[256] = "";
  int login = 0;

  strcpy(name, argv[1]);
  strcpy(password, argv[2]);
	
  //connect_request(&sockfd, &server_addr);
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    exit(1);
  }
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);
	
  if(connect(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  //send(sockfd, name, strlen(name) + 1, 0);
  //send(sockfd, password, strlen(password) + 1, 0);
  
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(0, &master);
  FD_SET(sockfd, &master);
  fdmax = sockfd;
	
  while(1) {
    read_fds = master;
    
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      perror("select");
      exit(EXIT_FAILURE);
    }
		
    for (i = 0; i <= fdmax; i++ ) {
      if (FD_ISSET(i, &read_fds)) {
	//send_recv(i, sockfd);
	  char send_buf[BUFSIZE];
	  char recv_buf[BUFSIZE];
	  char mess_buf[BUFSIZE];
	  int nbyte_recvd;
	
	  if (i == 0) {
	    fgets(send_buf, BUFSIZE, stdin);
	    send_buf[strlen(send_buf)-1] = '\0';
	    
	    if (strcmp(send_buf , "#quit") == 0) {
	      return 0;
	    }
	    else {
	      if (login == 0) {
		//strcpy(mess_buf, 
	      }
	      else {
		strcpy(mess_buf, name);
		strcat(mess_buf, ": ");
		strcat(mess_buf, send_buf);

		send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	      }
	    }
	  }
	  else {
	    nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0);
	    recv_buf[nbyte_recvd] = '\0';
	    printf("%s\n" , recv_buf);
	    fflush(stdout);
	  }
      }
    }
  }
  
  printf("client-quited\n");
  close(sockfd);
  
  return 0;
}
