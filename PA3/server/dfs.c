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
#include <sys/stat.h>

#define MAXLINE 4096

struct Users{
	char * username[10];
	char * password[10];
}users;

struct Server_Info{
	char server_name[20];
	char port_number[20];
	int server_number;
}server_info;


int read_config(char * conf_filename);
int service_request(int conn);

////MAIN/////////
int main (int argc, char * argv[] ){
  char conf_filename[20] = "dfs.conf";
  pid_t pid;
	int sock, conn, port;//This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	int remote_len = sizeof(remote);
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char command[10];

  if(argc != 3){
    printf("usage: <server_name> <port_number>\n");
    exit(1);
  }
	strcpy(server_info.server_name, argv[1]);
  strcpy(server_info.port_number, argv[2]);
	server_info.server_number = server_info.server_name[3] - '0';
  if( read_config(conf_filename) < 0){
		perror("error in read_config\n");
	}

  bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(server_info.port_number));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("unable to create socket");
  }

  if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
  {
    perror("unable to bind socket\n");
    exit(1);
  }

  if( listen(sock, 1) < 0)
	{
		printf("call to listen failed");
	}
  while(1)
  {
    if ((conn = accept(sock, NULL, NULL)) < 0)
      perror("Error calling accept");
    if ((pid = fork()) == 0)
    {
      if(close(sock) < 0)
        perror("error closing the sock");

      if(service_request(conn) < 0)
				printf("%s\n","error in service_request");
      if(close(conn) < 0)
        perror("error closing conn");

      exit(EXIT_SUCCESS);
    }
    if(close(conn) < 0)
      perror("error closing conn in parent");
    waitpid(-1, NULL, WNOHANG);
  }


  return 0;
}

int service_request(int conn){
  char buf[MAXLINE];
	int buflen = 0;
	int part_number;
	int nbytes;
	char command[30];
	char file_name[30];
	char client_username[30];
	char client_password[30];
  if(nbytes = read(conn, (char*)&buflen, sizeof(buflen)) < 0)
		perror("error reading");
	buflen = ntohl(buflen);
	printf("The client says %d\n", buflen);
  if((nbytes = read(conn, buf, buflen)) < 0)
		perror("error reading");
	printf("The client says %s\n", buf);
	char temp[4];
	strncpy(temp,buf, 4);
	if(strcmp(temp,"list")==0)
			sscanf(buf, "%s %s %s",command, client_username, client_password );
	else
		sscanf(buf, "%s %s %s %s",command, file_name,client_username,
		 client_password );
	int flag = 0;
	for(int i=0; i<10; i++){
		if(users.username[i] == NULL)
			break;
		if(strcmp(users.username[i],client_username)==0 &&
		 strcmp(users.password[i], client_password)==0 ){
			 flag = 1;
			 break;
		 }
		 else
		 	continue;
	}
	if(flag == 0){
		printf("%s\n","Invalid Username/Password. Please try again." );
		return -1;
	}
	struct stat st = {0};
	char dir[30];
	sprintf(dir, "./%s/%s", server_info.server_name, client_username );
	printf("%s\n", dir);
	if (stat(dir, &st) == -1){
		mkdir(dir, 0777);
	}

	// PUT //
	if(strcmp(command, "put") == 0){
		bzero(buf,sizeof(buf));
		for(int i=0; i<2; i++){
			buflen = 0;
			if(read(conn, &part_number, sizeof(part_number)) < 0)
				printf("Error Receiving the file\n");
			printf("%d\n",part_number );

			if(read(conn, &buflen, sizeof(buflen)) < 0)
				printf("Error Receiving the file\n");
			buflen = ntohl(buflen);
			printf("%d\n",buflen );
			if(read(conn, buf, buflen) < 0)
				printf("Error Receiving the file\n");
			printf("%s\n",buf );
			char dir1[50];
			sprintf(dir1, "%s/%s.%d",dir, file_name, part_number+1);
			printf("%s\n",dir1 );
			FILE *f = fopen(dir1, "wb");
			if(f == NULL)
				perror("Error opening the file!\n");
			// 	return -1;
			// }
			fprintf(f, "%s", buf);
			fclose(f);
			bzero(buf,sizeof(buf));
			buflen = 0;
			part_number = 0;
		}
	}

// GET //
	if(strcmp(command, "get") == 0){
		printf("%s\n","in LIST:" );
		char str[200];
		DIR *d;
		struct dirent *dir2;

		if((d = opendir(dir)) == NULL)
			perror("error in opendir");
		if (d)
		{
			while((dir2 = readdir(d)) != NULL)
			{
				strcat(str, dir2->d_name);
				strcat(str, "\n");
			}
			printf("%s", str );
			closedir(d);
		}
		int list_len = htonl(strlen(str));
		if (write(conn, (char*)&list_len, sizeof(list_len)) < 0)
		{
			perror("sendto");
			exit(1);
		}
		if (write(conn, str, strlen(str)) < 0)
		{
			perror("sendto");
			exit(1);
		}
		char * f_name[50] = {NULL};
		char *token = strtok(str, "\n");
		int j = 0;
		while(token!=NULL){
			size_t token_size = strlen(token);
			int counter = 0;
			if(isdigit(token[strlen(token)-1])){
				f_name[j] = calloc(strlen(token), sizeof(char));
				strncpy(f_name[j],token, strlen(token));
						}
				token = strtok(NULL, "\n");
				j++;
		}
		int file_count = 0;
		int flag1,flag2,flag3,flag4 = 0;
		for(int i=0;i<50; i++){
			if(f_name[i] != NULL && strstr(f_name[i],file_name) != NULL){
				char c1 = (f_name[i][strlen(f_name[i])-1]);
				int c2 = c1 - '0';
				if(c2==1 && flag1 == 0 ){
					flag1 = 1;
					file_count++;
				}
				if(c2==2 && flag2 == 0){
					printf("%s\n","here" );
					flag2 = 1;
					file_count++;
				}
				if(c2==3 && flag3 == 0){
					flag3 = 1;
					file_count++;
				}
				if(c2==4 && flag4 == 0){
					flag4 = 1;
					file_count++;
				}
			}
		}
		int flag[4];
		if(write(conn, &file_count, sizeof(file_count)) < 0)
			perror("error writing");

		for(int i=0; i<50; i++){
			if(f_name[i] != NULL && strstr(f_name[i],file_name) != NULL){
			char cc = (f_name[i][strlen(f_name[i])-1]);
			int cc2 = cc - '0';
			printf("%s %s\n", f_name[i], file_name );
			char file_dir[50];
			sprintf(file_dir, "%s/%s", dir, f_name[i] );
			printf("%s\n", file_dir );
			FILE *fp = fopen(file_dir, "r+b");
			if(fp == NULL)
				perror("file error");
			fseek(fp, 0l, SEEK_END );
			long file_size = ftell(fp);
			rewind(fp);
			int c;
			int n = 0;
			bzero(buf,sizeof(buf));
			if(fp != NULL){
				while((c = fgetc(fp)) != EOF ){
					buf[n++] = (char) c;
				}
			}
			printf("%c\n", f_name[i][strlen(f_name[i])-1] );
			if(flag[cc2-1] != 1){
			if(write(conn, &cc, sizeof(char)) < 0)
				perror("error writing");
			if(write(conn, (char *)&file_size, sizeof(file_size)) < 0)
				perror("error writing");
			printf("%s\n", buf );
			if(write(conn, buf, file_size) < 0)
				perror("error writing");
			flag[cc2-1] = 1;
			}
			fclose(fp);
			}
		}
	}

// lIST //
	if(strcmp(command, "list") == 0){
		printf("%s\n","in LIST:" );
		char str[200];
		DIR *d;
		struct dirent *dir2;

		if((d = opendir(dir)) == NULL)
			perror("error in opendir");
		if (d)
		{
			while((dir2 = readdir(d)) != NULL)
			{
				strcat(str, dir2->d_name);
				strcat(str, "\n");
			}
			printf("%s", str );
			closedir(d);
		}
		int list_len = htonl(strlen(str));
		if (write(conn, (char*)&list_len, sizeof(list_len)) < 0)
		{
			perror("sendto");
			exit(1);
		}
		if (write(conn, str, strlen(str)) < 0)
		{
			perror("sendto");
			exit(1);
		}
	}
	return 0;
}


int read_config(char * conf_filename)
{
	 char buffer[100];
	 FILE * fp;
	 char string1[20];
	 char string2[20];
	 char string3[50];
	 if((fp = fopen(conf_filename, "r")) == NULL)
	 		perror("error opening the config file");

	int count = 0;
	while(!feof(fp))
	{
		fgets(buffer, 100, fp);
    users.username[count] = calloc(sizeof(buffer), sizeof(char));
    users.password[count] = calloc(sizeof(buffer), sizeof(char));
    sscanf(buffer, "%s %s", users.username[count], users.password[count]);
    printf("%s %s\n",users.username[count], users.password[count] );
    count++;
  }
	return 0;
}
