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
			
int main()
{
  int sockfd, fdmax, i;
  struct sockaddr_in server_addr;
  fd_set master;
  fd_set read_fds;
  char username[20] = "";
  char unsecret[20] = "";
  int login = 0;
  int state = 0;
	
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
  
  FD_ZERO(&master);
  FD_ZERO(&read_fds);
  FD_SET(0, &master);
  FD_SET(sockfd, &master);
  fdmax = sockfd;

  char send_buf[BUFSIZE];
  char recv_buf[BUFSIZE];
	
  while(1) {
    read_fds = master;
    
    if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1){
      perror("select");
      exit(EXIT_FAILURE);
      }
		
    for (i = 0; i <= fdmax; i++ ) {
      if (FD_ISSET(i, &read_fds)) {
	//send_recv(i, sockfd);
	char mess_buf[BUFSIZE];
	int nbyte_recvd;
	char* token = NULL;
	
	if (i == 0) {
	  fgets(send_buf, BUFSIZE, stdin);
	  
	  if (strcmp(send_buf, "\n") != 0) {
	    send_buf[strlen(send_buf)-1] = '\0';
	    strcpy(mess_buf, send_buf);
	    
	    if (strcmp(send_buf, "#quit") == 0) {
	      return 0;
	    }
	    else if (strcmp(send_buf, "#noprivate") == 0) {
	      state = 0;
	    }
	    else if (strcmp(send_buf, "#3") == 0) {
	      strcpy(mess_buf, "showall/#");
	      send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	      state = 3;
	    }
	    else if (state == 1) {
	      strcpy(mess_buf, "signup/");
	      strcat(mess_buf, send_buf);
	      send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	    }
	    else if (state == 2) {
	      strcpy(mess_buf, "login/");
	      strcat(mess_buf, send_buf);
	      send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	      token = strtok(send_buf, "/");
	      strcpy(username, token);
	    }
	    else if (strcmp(send_buf, "#4") == 0) {
	      strcpy(mess_buf, "logout/#");
	      send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	      state = 4;
	    }
	    else if (login == 1) {
	      if (state == 5) {
		strcpy(mess_buf, send_buf);
		
		if ((token = strtok(mess_buf, "/")) != NULL) {
		  if (strcmp(token, "#sendfile") == 0) {
		    if ((token = strtok(NULL, "/")) != NULL) {
		      
		    }
		  }
		  else {	   
		    strcpy(mess_buf, "#private/");
		    strcat(mess_buf, username);
		    strcat(mess_buf, "/");
		    strcat(mess_buf, unsecret);
		    strcat(mess_buf, "/");
		    strcat(mess_buf, send_buf);
		    send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
		  }
		}
	      }
	      
	      if (state != 5) {
		if ((token = strtok(mess_buf, "/")) != NULL) {
		  if (strcmp(token, "#private") == 0) {
		    if ((token = strtok(NULL, "/")) != NULL) {
		      //strcpy(send_buf, "#private/");
		      //strcat(send_buf, username);
		      //strcat(send_buf, "/");
		      //strcat(send_buf, token);
		      strcpy(unsecret, token);
		      state = 5;
		    }
		  }
		}
	      }
	      
	      if (state == 0) {
		strcpy(mess_buf, username);
		strcat(mess_buf, ": ");
		strcat(mess_buf, send_buf);

		send(sockfd, mess_buf, strlen(mess_buf) + 1, 0);
	      }
	    }

	    if (strcmp(send_buf, "#1") == 0) state = 1;
	    if (strcmp(send_buf, "#2") == 0) state = 2;
	    //if (strcmp(send_buf, "#3") == 0) state = 3;
	    //if (strcmp(send_buf, "#4") == 0) state = 4;
	  }

	  fflush(stdin);
	}
	else {
	  if ((nbyte_recvd = recv(sockfd, recv_buf, BUFSIZE, 0)) <= 0) {
	    if (nbyte_recvd == 0) {
	      printf("Server is closed!\n");
	    }
	    else {
	      perror("recv");
	    }

	    close(sockfd);
	    return 0;
	  }
	  else {
	    recv_buf[nbyte_recvd] = '\0';

	    if (strcmp(recv_buf, "#reset") == 0) {
	      printf("Username not available!\n");
	      state = 0;
	    }
	    else if (state == 3) {
	      char* token = NULL;
	      
	      if ((token = strtok(recv_buf, "/")) != NULL) {
		printf("  %s\n", token);
		
		while ((token = strtok(NULL, "/")) != NULL) {
		  printf("  %s\n", token);
		}
	      }
	      
	      state = 0;
	    }
	    else if (state == 1 && strcmp(recv_buf, "fail") == 0) {
	      printf("Account already exists.\n");
	      state = 0;
	    }
	    else if (state == 1 && strcmp(recv_buf, "success") == 0) {
	      printf("The account has been created successfully.\n");
	      state = 0;
	    }
	    else if (state == 2 && strcmp(recv_buf, "success") == 0) {
	      login = 1;
	      state = 0;
	      printf("Correct!\n");
	      send(sockfd, username, sizeof(username) + 1, 0);
	    }
	    else {
	      printf("%s\n", recv_buf);
	    }
	    
	    fflush(stdout);
	  }
	}
      }
    }
  }
  
  printf("client-quited\n");
  close(sockfd);
  
  return 0;
}
