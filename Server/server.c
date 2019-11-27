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
#define SOCKLEN 100

struct account {
  char username[20];
  char password[20];
  int sockfd;
  int iscreate;
};

int loginUser(struct account* acc, char* username, char* password, int sockfd)
{
  for (int i = 0; i < SOCKLEN; i++) {
    //printf("%s/%s/%d\n", acc[i].username, acc[i].password, acc[i].sockfd);
    if (strcmp(acc[i].username, username) == 0 && strcmp(acc[i].password, password) == 0) {
      acc[i].sockfd = sockfd;
      return 0;
    }
  }

  return -1;
}

int checkUser(struct account* acc, char* username)
{
  for (int i = 0; i < SOCKLEN; i++) {
    if (strcmp(acc[i].username, username) == 0) return -1;
  }
  return 0;
}

int saveData(struct account* acc)
{
  FILE* f = NULL;

  if ((f = fopen("data", "wb")) == NULL) {
    printf("Error! opening file\n");
    return -1;
  }

  for (int i = 0; i < SOCKLEN; i++) {
    fwrite(&acc[i], sizeof(struct account), 1, f);
  }

  if (f) fclose(f);

  return 0;
}

int loadData(struct account* acc)
{
  FILE* f = NULL;

  if ((f = fopen("data", "rb")) == NULL) {
    if ((f = fopen("data", "wb")) == NULL) {
      printf("Error! opening file for create\n");
      return -1;
    }
    else {
      if (f) fclose(f);

      if ((f = fopen("data", "rb")) == NULL) {
	printf("Error! opening file for read\n");
	return -1;
      }
    }
  }

  for (int i = 0; i < SOCKLEN; i++) {
    fread(&acc[i], sizeof(struct account), 1, f);
  }

  if (f) fclose(f);

  return 0;
}

int main()
{
  fd_set master;
  fd_set read_fds;
  int fdmax, i;
  int sockfd = 0;
  struct sockaddr_in server_addr, client_addr;
  int opt = 1;
  char buf[BUFSIZE];
  int login[SOCKLEN];
  struct account acc[SOCKLEN];
  char username[20];
  char password[20];

  // Create data
  for (i = 0; i < SOCKLEN; i++) {
    strcpy(acc[i].username, "");
    strcpy(acc[i].password, "");
    acc[i].iscreate = -1;
  }

  loadData(acc);

  for (i = 0; i < SOCKLEN; i++) {
    acc[i].sockfd = -1;
    login[i] = -1;
  }
  
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

  if (listen(sockfd, SOCKLEN) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("\nServer Waiting for client on port %d\n\n", PORT);
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
	if (i == 0) {
	  printf("Go there\n");
	  fgets(buf, BUFSIZE, stdin);

	  if (strcmp(buf, "#quit\n") == 0) {
	    saveData(acc);
	    close(sockfd);
	    return 0;
	  }

	  fflush(stdin);
	}
	else if (i == sockfd) {
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
	  }
	}
	else {
	  // Send and receive
	  int nbytes_recvd, j;
	  char recv_buf[BUFSIZE], buf[BUFSIZE];
	
	  if ((nbytes_recvd = recv(i, recv_buf, BUFSIZE, 0)) <= 0) {
	    if (nbytes_recvd == 0) {
	      if (login[i] >= 0) {
		char send_buf[BUFSIZE];

		for (j = 0; j < SOCKLEN; j++) {
		  if (acc[j].sockfd == i) {
		    printf("%s log out\n", acc[j].username);
		    strcpy(send_buf, acc[j].username);
		    strcat(send_buf, " log out");
		    acc[j].sockfd = -1;
		    break;
		  }
		}

	      
		for(j = 0; j <= fdmax; j++) {
		  if (FD_ISSET(j, &master)) {
		    if (j != sockfd && j != i && login[j] == 1) {
		      if (send(j, send_buf, sizeof(send_buf), 0) == -1) {
			perror("send");
		      }
		    }
		  }
		}
	      }
	    }
	    else {
	      perror("recv");
	    }

	    login[i] = -1;
	    close(i);
	    FD_CLR(i, &master);
	  }
	  else {
	    if (login[i] == -1) {
	      char* token = NULL;

	      token = strtok(recv_buf, "/");
	      if (strcmp(token, "signup") == 0) {
		// Check username, password
		if ((token = strtok(NULL, "/")) == NULL) {
		  send(i, "failsignup1", 12, 0);
		}
		else {
		  strcpy(username, token);

		  if ((token = strtok(NULL, "/")) == NULL) {
		    send(i, "failsignup2", 12, 0);
		  }
		  else {
		    strcpy(password, token);

		    if (checkUser(acc, username) == 0) {
		      for (j = 0; j < SOCKLEN; j++) {
			if (acc[j].sockfd == -1 && acc[j].iscreate == -1) {
			  strcpy(acc[j].username, username);
			  strcpy(acc[j].password, password);
			  acc[j].sockfd = i;
			  acc[j].iscreate = 0;
			  saveData(acc);
			  send(i, "success", 8, 0);
			  break;
			}
		      }
		    }
		    else {
		      char mess_buf[BUFSIZE];
		      strcpy(mess_buf, "Username is duplicated!");
		      send(i, mess_buf, sizeof(mess_buf), 0);
		    }
		  }
		}
	      }
	      else if (strcmp(token, "login") == 0) {
		// Check username, password
		if ((token = strtok(NULL, "/")) == NULL) {
		  send(i, "faillogin1", 11, 0);
		}
		else {
		  strcpy(username, token);
		  
		  if ((token = strtok(NULL, "/")) == NULL) {;
		    send(i, "faillogin2", 11, 0);
		  }
		  else {
		    strcpy(password, token);
		    if (loginUser(acc, username, password, i) == 0) {
		      send(i, "success", 8, 0);
		      printf("%s log in\n", username);
		      
		      login[i] = 0;
		    }
		    else {
		      send(i, "faillogin3", 11, 0);
		    }
		  }
		}
	      }
	    }
	    else if (login[i] == 0) {
	      char send_buf[BUFSIZE];
		
	      strcpy(send_buf, recv_buf);
	      strcat(send_buf, " log in");
	      
	      for(j = 0; j <= fdmax; j++) {
		if (FD_ISSET(j, &master)) {
		  if (j != sockfd && j != i && login[j] == 1) {
		    if (send(j, send_buf, sizeof(send_buf), 0) == -1) {
		      perror("send");
		    }
		  }
		}
	      }

	      login[i] = 1;
	    }
	    else if (login[i] == 1) {
	      printf("%s\n", recv_buf);

	      // Send all
	      for(j = 0; j <= fdmax; j++) {
		if (FD_ISSET(j, &master)) {
		  if (j != sockfd && j != i && login[j] == 1) {
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
  }

  return 0;
}
