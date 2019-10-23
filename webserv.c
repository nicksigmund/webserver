/*
Nicholas Sigmund
webserv.c
*/


#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERV_PORT 8031
#define BUFFER_SIZE 1024

void *respond(void *childsock);
void handle_GET(unsigned int childsock, char* buff);
void handle_POST(unsigned int childsock, char* buff);

int main(){
	int status;
	unsigned int servsoc;
	unsigned int childsock;
	pthread_t tid;

	struct sockaddr_in serv;
	struct sockaddr_in cli;

	servsoc = socket(AF_INET, SOCK_STREAM, 0);
	if(!servsoc){
		perror("Failed to create socket");
		exit(1);
	}

	serv.sin_family = AF_INET;
	serv.sin_port = htons((int)(SERV_PORT));
	serv.sin_addr.s_addr = htons(INADDR_ANY);

	status = setsockopt(servsoc, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if(status < 0){
		perror("setsockopt failure");
		exit(1);
	}

	char host[50];
	gethostname(host, sizeof(host));
	puts(host);

	struct hostent *host_ent;
	host_ent = gethostbyname("localhost");
	char* IP = inet_ntoa(*((struct in_addr*)host_ent->h_addr_list[0]));
	puts(IP);

	status = bind(servsoc, (struct sockaddr* )&serv, sizeof(serv));
	if(status < 0){
		perror("Failed to bind");
		exit(1);
	}

	status = listen(servsoc, 5);
	if (status < 0){
		perror("failed on listen");
		exit(1);
	}

	int n = sizeof(cli);
	while(1){
		childsock = accept(servsoc, (struct sockaddr*)&cli, &n);
		if(childsock){
			pthread_create(&tid, NULL, &respond, (void *)&childsock);
			pthread_join(tid, NULL);
		}
		else
			close(childsock);
	}
	close(servsoc);
}

void *respond(void *sock){
	unsigned int childsock = *((unsigned int*)sock);
	char buff[BUFFER_SIZE];
	

	memset(buff, '\0', BUFFER_SIZE);

	recv(childsock, buff, BUFFER_SIZE, 0);
	//printf("Recieved request:\n%s\n", buff);
	if(strstr(buff, "GET") != NULL){
		handle_GET(childsock, buff);
	}
	else if(strstr(buff, "POST") != NULL){
		;
	}
	
	close(childsock);
	
}

void handle_GET(unsigned int childsock, char* buff){
	struct stat fs;
	int fsize, fd;
	char content_length[20];
	char ftype[20];
	char *fname, *tmp;

	tmp = strtok(buff, " ");
	fname = strtok(NULL, " ");

	if(fname == NULL || strlen(fname) < 2)
		fd = -1;
	else
		fd = open(&fname[1], O_RDONLY, S_IREAD | S_IWRITE);

	if(fd <= 0){
			fstat(0, &fs);
		strcpy(buff, "HTTP / 1.0 404 Not Found\nContent - Type:text/html\n\n<html><body><h1>FILE NOT FOUND</h1></body></html>");
	}
	else {
		fstat(fd, &fs);
		sprintf(content_length, "%zd", fs.st_size);

		if(strstr(fname, "gif") != NULL)
			strcpy(ftype, "image/gif");
		else if (strstr(fname, ".jpg") != NULL)
			strcpy(ftype, "image/jpg");
		else if (strstr(fname, ".html") != NULL || strstr(fname, ".htm") != NULL)
			strcpy(ftype, "text/html");
		else
			strcpy(ftype, "text/plain");
		strcpy(buff, "HTTP/1.1 200 OK\r\nContent-Length: ");
		strcat(buff, content_length);
		strcat(buff, "\r\n");
		strcat(buff, "Content-Type: ");
		strcat(buff, ftype);
	}
	strcat(buff, "\r\n\r\n");		//ending of HTTP header
	send(childsock, buff, strlen(buff), 0);	//send header
	//puts(buff);
	int buff_len;
	// this will be ignored if 404 error ==> fs.st_size will be 0
	// sends file
	for (int i = 0; i < fs.st_size; i += buff_len) {
		buff_len = read(fd, buff, BUFFER_SIZE);
		//	puts(buff);
		send(childsock, buff, buff_len, 0);
	}
	if(fd > 0)
		close(fd);
}

void handle_POST(unsigned int childsock, char* buff){
	//puts(buff);
	char* qs = strstr(buff, "\r\n\r\n");
	puts(qs);
	strcpy(buff, "HTTP / 1.0 404 Not Found\nContent - Type:text/html\n\n<html><body><h1>FILE NOT FOUND</h1></body></html>");

	send(childsock, buff, strlen(buff), 0);
}