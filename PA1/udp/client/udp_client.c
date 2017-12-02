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
#include <errno.h>

#define MAXBUFSIZE 6000

int main (int argc, char * argv[])
{

	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];
	char file_buffer[MAXBUFSIZE];
	struct sockaddr_in remote;              //"Internet socket address structure"
	int remote_len = sizeof(remote);

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}
	/******************
	  Here we populate a sockaddr_in struct with
	  information regarding where we'd like to send our packet
	  i.e the Server.
	 ******************/
	bzero(&remote,sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address

	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	//fcntl(sock, F_SETFL, 0_NONBLOCK);

	 printf("This is a udp connection. Please choose one of the followings:\n");
	 printf("get[file_name]\nput[file_name]\ndelete[file_name]\nls\nexit \n\n");
	char user_input[100], command[10], file_name[20];
	fgets(user_input, 30, stdin);
	sscanf(user_input, "%s %s", command, file_name);
	if (sendto(sock, user_input, strlen(user_input), 0, (struct sockaddr *)&remote, remote_len)==-1)
	{
		perror("sendto");
		exit(1);
	}

	if((strcmp(command, "get")) == 0)
	{
		bzero(buffer,sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len);
		if (nbytes >= 0) {
			printf("received message!\n");
		}
		FILE *file;
		file = fopen(file_name,"w");

		if(fwrite(buffer, nbytes, 1, file)<0)
		{
			printf("error writing file");
			exit(1);
		}
		fclose(file);
	}

	else if((strcmp(command, "put")) == 0)
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
		printf("%lu\n", file_size);
		fseek(file, 0 , SEEK_SET);
		bzero(file_buffer,sizeof(file_buffer));
		if((n_bytes = fread(file_buffer, 1, file_size, file))<=0)
		{
			printf("Couldn't copy file into the buffer");
			exit(1);
		}
		if(sendto(sock, file_buffer, n_bytes, 0, (struct sockaddr*)&remote, remote_len)<0)
			perror("sendto");
		fclose(file);

	}
	else if ((strcmp(command, "ls")) == 0)
	{
		bzero(buffer,sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len);
		if (nbytes >= 0) {
			printf("received message: \"%s\"\n", buffer);
		}
	}
	else if ((strcmp(command, "exit")) == 0)
	{
		bzero(buffer,sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len);
		if (nbytes >= 0) {
			printf("received message: \"%s\"\n", buffer);
		}
	}
	else
	{
		bzero(buffer,sizeof(buffer));
		nbytes = recvfrom(sock, buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_len);
		if (nbytes >= 0) {
			printf("received message: \"%s\"\n", buffer);
		}
	}
	close(sock);
	return(0);
}
