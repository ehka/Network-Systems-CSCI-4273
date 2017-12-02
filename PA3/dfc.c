
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
#include <openssl/md5.h>

#define MAXLINE 4096

struct Servers_Info{
	char * name[4];
	char * ip[4];
	char * port[4];
	char * username;
	char * password;
	char * comp_files[10];
}servers_info;

// functions
int read_config(char * conf_filename);
int get_func(char * file_name,int sock[]);
int put_func(char * file_name, int sock[]);
int list_func(int sock[]);
int check_user(int sock[]);


// main
int main(int argc, char *argv[]){
	char conf_filename[100];
	int nbytes;                             // number of bytes send by sendto()
	int sock[4];                               //this will be our socket
	struct sockaddr_in dfs[4], tt;              //"Internet socket address structure"
	int remote_len = sizeof(tt);

	if(argc != 2){
		printf("usage: <config_file>\n");
		EXIT_FAILURE;
	}
	else{
		strcpy(conf_filename, argv[1]);
	}
	if( read_config(conf_filename) < 0){
		perror("error in read_config\n");
	}
	for (int i=0; i<4; i++){
		bzero(&dfs[i],sizeof(dfs[i]));               //zero the struct
		dfs[i].sin_family = AF_INET;                 //address family
		dfs[i].sin_port = htons(atoi(servers_info.port[i]));      //sets port to network byte order
		dfs[i].sin_addr.s_addr = inet_addr(servers_info.ip[i]); //sets remote IP address
		if ((sock[i] = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			printf("unable to create socket");
		}
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		if (setsockopt (sock[i], SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
						sizeof(timeout)) < 0)
				error("setsockopt failed\n");
		if (connect(sock[i], (struct sockaddr *) &dfs[i], sizeof(dfs[i]))<0) {
		 perror("Problem in connecting to the server");
		 exit(3);
		}
	}
	char user_input[100], command[10], file_name[20], first_write[100];
	fgets(user_input, 30, stdin);
	sscanf(user_input, "%s %s", command, file_name);
	strtok(user_input, "\n");
	sprintf(first_write, "%s %s %s", user_input, servers_info.username, servers_info.password);
	printf("%s\n", first_write );
	int first_write_len = htonl(strlen(first_write));
	for(int i=0; i<4; i++){
		if (write(sock[i], (char*)&first_write_len, sizeof(first_write_len)) < 0)
		{
			perror("sendto");
			exit(1);
		}
		if (write(sock[i], first_write, strlen(first_write)) < 0)
		{
			perror("sendto");
			exit(1);
		}
	}

	if((strcmp(command, "get")) == 0){
		if(get_func(file_name, sock) < 0)
			perror("error calling get_func function");
	}
	else if((strcmp(command, "put")) == 0){
		if(put_func(file_name, sock) < 0)
			perror("error calling put_func function");
	}
	else if((strcmp(command, "list")) == 0){
		if(list_func(sock) < 0)
			perror("error calling get_func function");
	}
	else
		exit(1);

	return 0;
}

// GET FUNCTION '''
int get_func(char * file_name,int sock[]){
	list_func(sock);
	int flag = 0;
	for(int i=0; i<10; i++){
		if(servers_info.comp_files[i] != NULL){
			if(strstr(servers_info.comp_files[i], file_name) != NULL){
				flag = 1;
				break;
			}
			else
				continue;
		}
	}
	if(flag == 0){
		printf("%s\n", "File is incomplete." );
		return 0;
	}
	printf("%s\n", "in get");
	char buf[MAXLINE];
	char buffer[4][MAXLINE];
	long buflen;
	int file_count[4];
	for(int i; i<4; i++){
	if(read(sock[i], &file_count[i], sizeof(file_count[i])) < 0)
		printf("Error Receiving the file\n");
	printf("%d\n", file_count[i] );
	}
	for(int i; i<4; i++){
		for(int j=0; j<file_count[i]; j++){
			char c;
			if(read(sock[i], &c, sizeof(char)) < 0)
				printf("Error Receiving the file\n");
			printf("%c\n", c );
			if(read(sock[i], (char*)&buflen, sizeof(buflen)) < 0)
				printf("Error Receiving the file\n");
			int nbytes;
			if((nbytes=read(sock[i], buf, buflen)) <= 0)
				printf("Error Receiving the file\n");
			printf("%s\n", buf );
			int fn = c - '0';
			if(fn == 1)
				strcpy(buffer[0], buf);
			if(fn == 2)
				strcpy(buffer[1], buf);
			if(fn == 3)
				strcpy(buffer[2], buf);
			if(fn == 4)
				strcpy(buffer[3], buf);
			bzero(buf, sizeof(buf));
			buflen = 0;
			}
		}
		for(int i=0; i<4; i++){
			printf("%s", buffer[i] );
		}
		printf("\n");
		FILE *fp = fopen(file_name, "wb");
		if(fp == NULL)
		{
			printf("file does not exist!");
			exit(1);
		}
		for(int i=0; i<4; i++)
					fprintf(fp, "%s", buffer[i]);
		return 0;
}

// PUT FUNCTION '''
int put_func(char * file_name, int sock[]){
	int n_bytes;
	FILE *fp;
	int block_size;
	long file_size;
	char * part[4];
	int flag = 0;
	size_t n = 0;
	int c = 0;
	unsigned char out[MD5_DIGEST_LENGTH];
	MD5_CTX mdContext;
	unsigned char * hash_value;
	unsigned long hash_r = 0;
	int md5_table[4][4][2] =
	{
		{
			{0,1},{1,2},{2,3},{3,0}
		},
		{
			{3,0},{0,1},{1,2},{2,3}
		},
		{
			{2,3},{3,0},{0,1},{1,2}
		},
		{
			{1,2},{2,3},{3,0},{0,1}
		}
	};
	fp = fopen(file_name, "r+b");
	if(fp == NULL)
	{
		printf("file does not exist!");
		exit(1);
	}
	fseek(fp, 0l, SEEK_END );
	file_size = ftell(fp);
	rewind(fp);
	block_size = file_size/4;
	printf("%d %d\n", file_size, block_size );
	if((file_size%2) != 0){
		flag = 1;
	}
	printf("flag %d\n",flag );
	for (int i=0; i < 4; i++)
		if (i == 3 && flag == 1){
					part[i] = calloc(sizeof(block_size)+1, sizeof(char));
				}
		else
					part[i] = calloc(sizeof(block_size), sizeof(char));

	int j = 0;
	MD5_Init (&mdContext);
	while((c = fgetc(fp)) != EOF ){
			part[j][n++] = (char) c;
			MD5_Update (&mdContext, &c, 1);
			if( j < 3){
				if(n >= block_size){
					j++;
					n = 0;
			}
		}
	}

	MD5_Final (out,&mdContext);
	char temp[4];
	hash_value = calloc(sizeof(MD5_DIGEST_LENGTH), sizeof(char));
	for (int i=0; i < MD5_DIGEST_LENGTH; i++){
		sprintf(temp, "%02x",(unsigned int)out[i]);
		strcat(hash_value, temp );
	}
	printf("%s\n", hash_value);
	char first_five[5];
	strncpy(first_five, hash_value, 5);
	hash_r =  (long)strtol(first_five, NULL, 16)%4;
	printf("%lu\n", hash_r );
	printf("%s\n",part[0] );
	printf("%s\n",part[1] );
	printf("%s\n",part[2] );
	printf("%s\n",part[3] );
	char ch;
	int extra_byte = 0;
	int data_len;
	int part_number;
	for(int j=0;j < 4; j++ ){
		for(int t=0; t < 2; t++){
			if (md5_table[hash_r][j][t] == 3 && flag == 1)
				extra_byte = 1;
			else extra_byte = 0;
			printf("%s  ",part[md5_table[hash_r][j][t]] );
			printf("%d\n",md5_table[hash_r][j][t]);
			data_len = strlen(part[md5_table[hash_r][j][t]]);
			printf("%d\n", data_len );
			data_len = htonl(data_len);
			part_number = (md5_table[hash_r][j][t]);
			if(write(sock[j], &part_number,sizeof(part_number)) < 0)
				perror("error writing1");
			if(write(sock[j], &data_len,sizeof(data_len)) < 0)
				perror("error writing2");
			if(write(sock[j], part[md5_table[hash_r][j][t]], strlen(part[md5_table[hash_r][j][t]])) < 1)
				perror("Eror sending the file.");
		}
	}

	fclose(fp);
	return 0;
}

// LIST FUNCTION '''
int list_func(int sock[]){
	char * f_name[50] = {NULL};
	int nbytes;
	int buflen;
	char buf[1024];
	int j = 0;
	for(int i=0; i<4; i++){
		if(nbytes = read(sock[i], (char*)&buflen, sizeof(buflen)) < 0)
			perror("error reading");
		buflen = ntohl(buflen);
		if(nbytes = read(sock[i], buf, buflen) < 0)
			perror("error reading");
		char *token = strtok(buf, "\n");
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
				bzero(buf,sizeof(buf));
				buflen = 0;
	}
	int counter = 0;
	char * fname_d[20] = {NULL};
	int flag = 0;
	printf("%s\n","All Files:" );
	for(int i=0; i<50; i++){
			if(f_name[i] != NULL){
			printf("%s\n",f_name[i] );
		}
	}
	printf("\n\n");
	for(int i=0; i<50; i++){
		if(f_name[i] != NULL){
			//printf("%s %d\n",f_name[i], i );
			for(int j=0; j<20; j++){
				if(fname_d[j] != NULL){
					if(strstr(f_name[i], fname_d[j]) != NULL){
						flag = 1;
						break;
					}
				else
					flag = 0;
				}
			}
			if(flag == 0){
					int t = sizeof(f_name[i]) - 3;
					fname_d[counter] = calloc(t, sizeof(char));
					strncpy(fname_d[counter], f_name[i], t);
					counter++;
			}
		}
	}
	int p_number[10][50] = {0};
	for(int i=0; i<20; i++){
		if(fname_d[i] != NULL){
		for(int j=0; j<50; j++){
			if(f_name[j] != NULL){
				if(strstr(f_name[j], fname_d[i]) != NULL){
						p_number[i][j] = f_name[j][strlen(f_name[j])-1] - '0';
				}
			}
		}
		}
	}
	printf("%s\n","Source Files:" );
	int f1,f2,f3,f4 = 0;
	int count = 0;
	for(int i=0;i<20;i++){
		if(fname_d[i] != NULL){
			for(int j=0;j<50; j++){
				if(p_number[i][j] != 0){
					if(p_number[i][j] == 1)
						f1 = 1;
					else if(p_number[i][j] == 2)
						f2 = 1;
					else if(p_number[i][j] == 3)
						f3 = 1;
					else if(p_number[i][j] == 4)
						f4 = 1;
					}
			}
		}
		else
			continue;

		if(f1 && f2 && f3 && f4){
			printf("%s\n",fname_d[i] );
			servers_info.comp_files[count] = calloc(strlen(fname_d[i]), sizeof(char));
			strcpy(servers_info.comp_files[count], fname_d[i]);
			count++;
		}
		else
			printf("%s [incomplete]\n", fname_d[i] );
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
		if( strstr(buffer, "Server")){
			servers_info.ip[count] = calloc(sizeof(buffer), sizeof(char));
			servers_info.name[count] = calloc(sizeof(buffer), sizeof(char));
			servers_info.port[count] = calloc(sizeof(buffer), sizeof(char));
			sscanf(buffer, "%s %s %s", string1, string2, string3);
			char * token = strtok(string3, ":");
			strcpy(servers_info.ip[count],token );
			token = strtok(NULL, ":");
			strcpy(servers_info.port[count],token );
			printf(" %s %s\n", servers_info.ip[count], servers_info.port[count] );
			count++;
		}
		else if(strstr(buffer,"Username")){
			servers_info.username = calloc(sizeof(buffer), sizeof(char));
			sscanf(buffer, "%s", string1);
			char * token = strtok(string1, ":");
			token = strtok(NULL, ":");
			strcpy(servers_info.username, token);
			printf("%s\n",servers_info.username );
		}
		else if(strstr(buffer, "Password")){
			servers_info.password = calloc(sizeof(buffer), sizeof(char));
			sscanf(buffer, "%s", string1);
			char * token = strtok(string1, ":");
			token = strtok(NULL, ":");
			strcpy(servers_info.password, token);
			printf("%s\n",servers_info.password );
		}
		else{
			printf("%s\n","wrong value in config file." );
			EXIT_FAILURE;
		}
	}
	fclose(fp);
	return 0;
}
