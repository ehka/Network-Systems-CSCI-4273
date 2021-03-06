#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread
#include<sys/wait.h>
#include<sys/types.h> 
#include<errno.h>
#include<sys/stat.h> 
#include<fcntl.h>

enum Req_Method { GET, UNSUPPORTED };
enum Req_Type {SIMPLE, FULL};

static char server_root[1000] = "/home/user/Documents/NetworkSystems/PA2/www";

struct ReqInfo{
	enum Req_Method method;
	char *resource;
	char *content_type;
	int content_length;
	int status;
	char *http_version;
	char * content_source;
};

struct Conf_info{
	int port_number;
	char *doc_root;
	char * def_webpage;
	char *cont_types[10];
	int timeout;
	char * cont_sources[10];
	int num_types;
};

struct Conf_info conf_info;

#define MAX_REQ_LINE (1024)


void read_config(struct Conf_info * conf_info, char * filename);
int service_request(int conn);
void InitReqInfo(struct ReqInfo * reqinfo);
void InitConfInfo(struct Conf_info * conf_info);
int Get_Request(int conn, struct ReqInfo *reqinfo);
ssize_t Readline(int sockd, void *vptr, size_t maxlen);
int Trim(char * buffer);
int Parse_HTTP_Header(char *buffer, struct ReqInfo * reqinfo);
int Check_Resource(struct ReqInfo * reqinfo);
void CleanURL(char * buffer);
int StrUpper(char *buffer);
int Output_HTTP_Headers(int conn, struct ReqInfo * reqinfo);
ssize_t Writeline(int sockd, const void *vptr, size_t n);
int Return_Resource(int conn, int resource, struct ReqInfo * reqinfo);
int Return_Error_Msg(int conn, struct ReqInfo * reqinfo);
void FreeReqInfo(struct ReqInfo * reqinfo);

int main(int argc, char *argv[])
{
	setbuf(stdout, NULL);
	InitConfInfo(&conf_info);
	char filename[20];
	strcpy(filename, "ws.conf.txt");
	read_config(&conf_info, filename);
	fflush( stdout );
	struct sockaddr_in server;
	pid_t pid;
	int sock, conn, port;

	if((sock = socket(AF_INET , SOCK_STREAM , 0)) < 0)
	{
		printf("unable to create socket");
	}
	bzero(&server,sizeof(server));                    //zero the struct
	server.sin_family = AF_INET;                   //address family
	server.sin_port = htons(conf_info.port_number);        //htons() sets the port # to network byte order
	server.sin_addr.s_addr = htonl(INADDR_ANY);           //supplies the IP address of the local machine
	
	if(bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0)
	{
		printf("unable to bind the socket");
	}
	if( listen(sock, 1) < 0)
	{
		printf("call to listen failed");
	}
	while(1)
	{
		if ((conn = accept(sock, NULL, NULL)) < 0)
			printf("Error calling accept");
			
		if ((pid = fork()) == 0)
		{
			if(close(sock) < 0)
				perror("error closing the sock");
				
			Service_Request(conn);
			if(close(conn) < 0)
				perror("error closing conn");
				
			exit(EXIT_SUCCESS);
		}
		for(int i=0; i<100000000; i++)
			i=i+1;
		if(close(conn) < 0)
			perror("error closing conn in parent");
		waitpid(-1, NULL, WNOHANG);
	}
	return EXIT_FAILURE;
}

void read_config(struct Conf_info * conf_info,char * conf_filename)
{
	FILE *fp;
	char buff[100];
	char header[100];
	char temp[100];
	char *ptr;
	int count_types = 0;
	if((fp = fopen(conf_filename, "r")) == NULL) {
		perror("error opening config file");
	}
	while(!feof(fp)){
		fgets(buff, 100, fp);
		if (buff[0] == '#'){
			sscanf(buff, "%s", header);
			continue;
		}
		if ( strstr(header, "service")){
			int ret = strtol(buff, &ptr, 0);
			conf_info->port_number = ret;
			memset(&header[0], 0,  sizeof(header));
		}
		if (strstr(header, "document")){
			conf_info->doc_root = calloc(sizeof(buff), sizeof(char));
			strncpy(conf_info->doc_root, buff, sizeof(buff));
			memset(&header[0], 0,  sizeof(header));
		}
		if ( strstr(header, "default")){
			conf_info->def_webpage = calloc(sizeof(buff), sizeof(char));
			strncpy(conf_info->def_webpage, buff, sizeof(buff));
			memset(&header[0], 0,  sizeof(header));
		}
		if ( strstr(header, "connection")){
			int ret = strtol(buff, &ptr, 0);
			conf_info->timeout = ret;
			memset(&header[0], 0,  sizeof(header));
		}
		if ( strstr(header, "Content-Type")){
			if(buff[0] == '\n')
				continue;
			strcpy(temp, buff);
			char *token = strtok(temp, " ");
			conf_info->cont_types[count_types] = calloc(sizeof(buff), sizeof(char));
			strncpy(conf_info->cont_types[count_types], token, sizeof(buff));
			token = strtok(NULL, " ");
			conf_info->cont_sources[count_types] = calloc(sizeof(buff), sizeof(char));
			strncpy(conf_info->cont_sources[count_types], token, sizeof(buff));
			count_types++;
			conf_info->num_types = count_types;
		}
	}
}

int Service_Request(int conn, struct Conf_info * conf_info)
{
	struct ReqInfo reqinfo;
	int resource = 0;
	InitReqInfo(&reqinfo);
	if(Get_Request(conn, &reqinfo) < 0)
		return -1;
	if(reqinfo.status == 200)
		if((resource = Check_Resource(&reqinfo)) < 0) {
			if (errno == EACCES)
				reqinfo.status = 404;
			else
				reqinfo.status = 404;
		}
	printf("%d\n", reqinfo.status);
	size_t size;
	struct stat st;
	fstat(resource, &st);
	size = st.st_size;
	reqinfo.content_length = size;
	if(reqinfo.status == 200){
		size_t size;
		struct stat st;
		fstat(resource, &st);
		size = st.st_size;
		reqinfo.content_length = size;
		Output_HTTP_Headers(conn, &reqinfo);
		if(Return_Resource(conn,resource,&reqinfo))
			printf("Something wrong returning resource.");
	}
	else{
		reqinfo.content_length = 0;
		Return_Error_Msg(conn, &reqinfo);
	}
	if(resource > 0)
		if(close(resource) < 0)
			printf("Error closing resource");
	FreeReqInfo(&reqinfo);
	return 0;
}

/* /////////////////////  */

void InitReqInfo(struct ReqInfo * reqinfo)
{
	reqinfo -> content_type = NULL;
	reqinfo -> content_length = 0;
	reqinfo -> resource = NULL;
	reqinfo -> method = UNSUPPORTED;
	reqinfo -> status = 200;
	reqinfo -> http_version = NULL;
	reqinfo -> content_source = NULL;
}

void InitConfInfo(struct Conf_info * conf_info)
{
	conf_info -> port_number = 8883;
	conf_info -> doc_root = server_root;
	conf_info -> def_webpage = NULL;
	conf_info -> timeout = 0;
	conf_info -> num_types = 0;
}

int Get_Request(int conn, struct ReqInfo * reqinfo)
{
	char buffer[MAX_REQ_LINE] = {0};
	Readline(conn, buffer, MAX_REQ_LINE-1);
	printf("%s\n", buffer);
	Trim(buffer);
	if(Parse_HTTP_Header(buffer, reqinfo))
		perror("error in parse");
	return 0; 
}

ssize_t Readline(int sockd, void *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *buffer;
	buffer = vptr;
	for( n=1; n< maxlen; n++){
		if ((rc = read(sockd, &c, 1)) == 1) {
			*buffer++ = c;
			if( c== '\n'){
				break;
			}
		}
		else if(rc == 0){
			if (n==1)
				return 0;
			else
				break;
		}
		else{
			if (errno == EINTR)
				continue;
			printf("Error in Readline()");
		}
	}
	*buffer = 0;
	return n;
}

int Trim(char * buffer)
{
	int n = strlen(buffer)-1;
	while(!isalnum(buffer[n]) && n >= 0)
		buffer[n--] = '\0';
	return 0;
}

ssize_t Writeline(int sockd, const void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nwritten;
	const char *buffer;
	buffer = vptr;
	nleft = n;
	
	while(nleft > 0){
		if((nwritten = write(sockd, buffer, nleft)) <= 0){
			if (errno == EINTR)
				nwritten = 0;
			else 
				printf("Error in Writeline()");
		}
		nleft -= nwritten;
		buffer += nwritten;
	}
	
	return n;
}

int Parse_HTTP_Header(char * buffer, struct ReqInfo * reqinfo)
{
	char *endptr;
	int len;
	char *token;
	char temp[100];
	if( !strncmp(buffer, "GET ", 4)){
		reqinfo ->method = GET;
		buffer +=4;
	}
	else{
		reqinfo -> method = UNSUPPORTED;
		reqinfo -> status = 501;
		return -1;
	}
	while(*buffer && isspace(*buffer))
		buffer++;
	endptr = strchr(buffer, ' ');
	if(endptr == NULL)
		len = strlen(buffer);
	else
		len = endptr - buffer;
	if( len == 0){
		reqinfo -> status = 400;
		return -1;
	}
	if(strstr(buffer,"HTTP/1.0"))
		reqinfo-> http_version = "HTTP/1.0";
	if(strstr(buffer, "HTTP/1.1"))
		reqinfo->http_version = "HTTP/1.1";
	if(buffer[0] == '/' && len==1){
		reqinfo -> resource = calloc(12, sizeof(char));
		strcpy(reqinfo -> resource, "/index.html");
		reqinfo -> content_type = calloc(4, sizeof(char));
		strncpy(reqinfo -> content_type, "html", 4);
	}
	else{
		reqinfo -> resource = calloc(len+1, sizeof(char));
		strncpy(reqinfo -> resource, buffer, len);
		strcpy(temp, reqinfo->resource);
		token = strtok(temp, ".");
		token = strtok(NULL, ".");
		reqinfo -> content_type = calloc(sizeof(token), sizeof(char));
		strncpy(reqinfo -> content_type, token, sizeof(token)+1);
	}
	printf("%s  %s\n", reqinfo->content_type, reqinfo->resource);
	return 0;
}

int Check_Resource(struct ReqInfo * reqinfo){
	CleanURL(reqinfo -> resource);
	int flag = 0;
	for( int i=0; i<conf_info.num_types; i++){
		if(strstr(conf_info.cont_types[i],reqinfo->content_type)!=NULL){
			reqinfo->content_source = conf_info.cont_sources[i];
			strtok(reqinfo->content_source, "\n");
			flag = 1;
			break;
		}
		else{
			continue;
		}
	}	
	if(flag == 0){
		reqinfo->status = 404;
		return -1;
	}
	char * file_path = conf_info.doc_root;
	file_path[strcspn(file_path,"\n")] = 0;
	strcat(file_path, reqinfo->resource); 
	return open(file_path, O_RDONLY);
}

void CleanURL(char * buffer){
	char asciinum[3] = {0};
	int i = 0, c;
	
	while(buffer[i]){
		if(buffer[i] == '+')
			buffer[i] = ' ';
		else if ( buffer[i] == '%'){
			asciinum[0] = buffer[i+1];
			asciinum[1] = buffer[i+2];
			buffer[i] = strtol(asciinum, NULL, 16);
			c = i+1;
			do{
				buffer[c] = buffer[c+2];
			} while (buffer[2+(c++)]);
		}
		++i;
	}
}

int StrUpper(char * buffer) {
	while (*buffer){
		*buffer = toupper(*buffer);
		++buffer;
	}
	return 0;
}

int Output_HTTP_Headers(int conn, struct ReqInfo * reqinfo){
	char buffer[100];
	sprintf(buffer, "%s %d OK\r\n",reqinfo->http_version, reqinfo->status);
	printf("%s", buffer);
	Writeline(conn, buffer, strlen(buffer));
	sprintf(buffer, "Content-Type: %s \r\n", reqinfo->content_source);
	printf("%s", buffer);
	Writeline(conn, buffer, strlen(buffer));
	sprintf(buffer, "Content-Length: %d \r\n", reqinfo->content_length);
	printf("%s", buffer);
	Writeline(conn, buffer, strlen(buffer));
	Writeline(conn, "\r\n", 2);
	return 0;
}


int Return_Resource(int conn, int resource, struct ReqInfo * reqinfo)
{
	char c;
	int i;
	while((i = read(resource, &c, 1)) ) {
		printf("%c",c);
		if (i < 0)
			printf("Error reading from file.");
		if(write(conn, &c, 1) < 1)
			printf("Eror sending the file.");
	}
	return 0;
}

int Return_Error_Msg(int conn, struct ReqInfo * reqinfo)
{
    char buffer[100];
	if(reqinfo->status == 400){
		sprintf(buffer, "%s %d Bad Request\r\n", reqinfo->http_version,
		reqinfo->status);
		Writeline(conn, buffer, strlen(buffer));
		if(!reqinfo->http_version){
			sprintf(buffer, "<html>\n<body>400 Bad Request Reason:"
			"invalid HTTP-Version</body>\n</html>\n\n");
			Writeline(conn, buffer, strlen(buffer)+1);
		}
		
		if(reqinfo->method != GET){
			sprintf(buffer, "<html>\n<body>400 Bad Request Reason:"
			"invalid Method</body></html>\n\n");
			Writeline(conn, buffer, strlen(buffer));
		}
		else{
			sprintf(buffer, "<html>\n<body>400 Bad Request Reason:"
			"invalid URL</body></html>\n\n");
			Writeline(conn, buffer, strlen(buffer));
		}
	}
	else if( reqinfo->status == 404){
		sprintf(buffer, "%s %d Not Found\r\n", reqinfo->http_version,
		reqinfo->status);
	Writeline(conn, buffer, strlen(buffer));
    sprintf(buffer, "<HTML>\n<HEAD>\n<TITLE>Server Error %d</TITLE>\n"
	            "</HEAD>\n\n", reqinfo->status);
    Writeline(conn, buffer, strlen(buffer));

    sprintf(buffer, "<BODY>\n<H1>Server Error %d</H1>\n", reqinfo->status);
    Writeline(conn, buffer, strlen(buffer));

    sprintf(buffer, "<P>Not Found Reason URL does not exist: %s.</P>\n"
	            "</BODY>\n</HTML>\n", reqinfo->resource);
    Writeline(conn, buffer, strlen(buffer));
	}
	else if(reqinfo->status == 501){
		sprintf(buffer, "%s %d Not implemented\r\n", reqinfo->http_version,
		reqinfo->status);
		Writeline(conn, buffer, strlen(buffer));
		sprintf(buffer, "<HTML>\n<HEAD>\n<TITLE>Server Error %d</TITLE>\n"
	            "</HEAD>\n\n", reqinfo->status);
	Writeline(conn, buffer, strlen(buffer));

	sprintf(buffer, "<BODY>\n<H1>Server Error %d</H1>\n", reqinfo->status);
	Writeline(conn, buffer, strlen(buffer));

	sprintf(buffer, "<P>501 Not Implemented: %s.</P>\n"
	            "</BODY>\n</HTML>\n", reqinfo->content_type);
	Writeline(conn, buffer, strlen(buffer));
	}
	else {
		sprintf(buffer, "%s %d Internal Server Error\r\n", reqinfo->http_version,
		reqinfo->status);
		Writeline(conn, buffer, strlen(buffer));
		sprintf(buffer, "<HTML>\n<HEAD>\n<TITLE>Server Error %d</TITLE>\n"
	            "</HEAD>\n\n", reqinfo->status);
	Writeline(conn, buffer, strlen(buffer));

	sprintf(buffer, "<BODY>\n<H1>Server Error %d</H1>\n", reqinfo->status);
	Writeline(conn, buffer, strlen(buffer));

	sprintf(buffer, "<P>501 Internal Server Error:cannot allocate memory.</P>\n"
	            "</BODY>\n</HTML>\n");
	Writeline(conn, buffer, strlen(buffer));
	}
	
	return 0;
}

void FreeReqInfo(struct ReqInfo * reqinfo)
{
	if(reqinfo-> content_type)
		free(reqinfo-> content_type);
	if(reqinfo->resource)
		free(reqinfo->resource);
	
}















