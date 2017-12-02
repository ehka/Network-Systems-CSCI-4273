#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include <sys/unistd.h>

#define MAXBUFSIZE 6000

int main (int argc, char * argv[] )
{


	int sock;                           //This will be our socket
	char file_buffer[MAXBUFSIZE]; 
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	int remote_len = sizeof(remote);
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char command[10];
	char file_name[20];
	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}
	
	//fcntl(sock, F_SETFL, 0_NONBLOCK);
	
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		perror("unable to bind socket\n");
		exit(1);
	}

	remote_length = sizeof(remote);

	//waits for an incoming message
	int flag = 1;
	while(flag)
{
	bzero(buffer,sizeof(buffer));
	printf("server is running...\n");
	nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	if(nbytes<0)
		printf("something went wrong");

	printf("The client says %s\n", buffer);
	
	sscanf(buffer, "%s %s",command, file_name);
	
	
	
	
	
	
	if(strcmp(command, "get") == 0)
	{
		int n_bytes;
		FILE *file;
		file = fopen(file_name, "r+b");
		if(file == NULL)
		{
			printf("file does not exist!");
			exit(1);
		}
		fseek(file,0,SEEK_END);
		size_t file_size = ftell(file);
		fseek(file, 0 , SEEK_SET);
		bzero(file_buffer,sizeof(file_buffer));
		if((n_bytes = fread(file_buffer, 1, file_size, file))<=0)
		{
			
			printf("Couldn't copy file into the buffer %d", n_bytes);
			exit(1); 
		}
		if(sendto(sock, file_buffer, n_bytes, 0, (struct sockaddr*)&remote, remote_length)<0){
			perror("sendto");
		}
		printf("message sent to client\n");
	}
	else if(strcmp(command, "put") == 0)
	{
		bzero(buffer,sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
		if (nbytes <= 0) {
			printf("Error Receiving the file\n");
		} 
		FILE *file;
		file = fopen(file_name,"w");
		if(fwrite(buffer, nbytes,1, file)<=0)
		{
			printf("error writing file\n");
			exit(1);
		}
		fclose(file);
		printf("Received!\n");
	}
	else if(strcmp(command, "delete") == 0)
	{
			FILE *file;
	file = fopen(file_name, "r");
	if(file == NULL)
	{
		printf("file does not exist!");
		exit(1);
	}
	char str[30];
	strcpy(str, file_name);
	strcat(str, " was deleted!");
	remove(file_name);
	sendto(sock, str, strlen(str), 0, (struct sockaddr*)&remote, remote_length);
	}
	else if(strcmp(command, "ls") == 0)
	{
		char str[200];
		DIR *d;
		struct dirent *dir;
		d = opendir(".");
		if (d)
		{
			while((dir = readdir(d)) != NULL)
			{
				strcat(str, dir->d_name);
				strcat(str, "\n");
			}
			closedir(d);
		}
		sendto(sock, str, strlen(str), 0, (struct sockaddr*)&remote, remote_length);
	}
	else if(strcmp(command, "exit") == 0)
	{
		char str[] = "exited the server";
		sendto(sock, str, strlen(str), 0, (struct sockaddr*)&remote, remote_length);
		exit(1);
	}
	else
	{
		char str[] = "The following command was not uderstood: ";
		char comm[20];
		strcpy(comm, command);
		strcat(str, comm);
		sendto(sock, str, strlen(str), 0, (struct sockaddr*)&remote, remote_length);
	}

}
	close(sock);
	return(0);
}

