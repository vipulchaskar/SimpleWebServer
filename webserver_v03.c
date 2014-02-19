#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>
#include<signal.h>

#include "httpheaders.h"

#define RESPSIZE 10000

char serverroot[] = "/usr/local/www";		//default www root folder
char notfound[] = "/usr/local/www/notfound.htm";//Error 404 page

int open_listener_socket();			//socket related functions
void bind_to_port(int,int);

int say(int,char *);				//request and response related functions
int hear(int,char *,int);

int catch_signal(int, void (*handler)(int));	//misc. functions
void handle_shutdown(int);
void error(char *);

int listener_d;						//global listener object


int main(int argc, char *argv[])
{
	char recvbuffer[500];
	int hearstatus;
	
	if(catch_signal(SIGINT,handle_shutdown) == -1)
		error("Can't set the interrupt handler");	//Ctrl+C interrupt used for closing

	listener_d = open_listener_socket();    //open socket, bind to port 80
	bind_to_port(listener_d,80);            //(requires root)

	if(listen(listener_d,10) == -1)         //and listen
		error("Can't listen.");

	struct sockaddr_storage client_addr;
	unsigned int address_size = sizeof(client_addr);

	puts("Waiting for connection");

	while(1)
	{
		int connect_d = accept(listener_d,(struct sockaddr *)&client_addr,&address_size);
		if(connect_d ==  -1)
			error("Can't create secondary socket.");

		if(fork() == 0)
		{
			FILE *page;
			char path[256]={'\0'};
			long length;
			char *response;
			char *contents;
			struct httpheaders headerdata;
			int resp=200;    //response type, 200 by default
			
			close(listener_d);

			hearstatus = hear(connect_d,recvbuffer,sizeof(recvbuffer));
			
			headerdata = parsehttp(recvbuffer);
			strcat(path,"/usr/local/www");      //default server root
			strcat(path,headerdata.path);
			
			if(path[strlen(path)-1]=='/') 
				strcat(path,"index.htm");
				
			else if(strstr(path,".")==0 && path[strlen(path)-1]!='/')
			{
					strcat(path,"/index.htm");
			}
				
			if((page=fopen(path,"r"))==NULL)
			{
				page=fopen("/usr/local/www/notfound.htm","r");
				resp=404;
			}
			
			if(page==NULL)
			{
				error("User request page not found.");
				resp=404;
			}
			
			fseek(page,0,SEEK_END);
			length = ftell(page);
			fseek(page,0,SEEK_SET);
			contents = malloc(length);
			
			if(contents)
			{
				fread(contents,1,length,page);
			}
			
			response = (char *)malloc(sizeof(char)*RESPSIZE);
			if(resp==200)
				strcpy(response,"HTTP/1.1 200 OK\r\nServer: ViServer\r\nConnection: close\r\n\r\n");
			else
				strcpy(response,"HTTP/1.1 400 Not Found\r\nServer: ViServer\r\nConnection: close\r\n\r\n");
			strcpy(response,contents);

			say(connect_d,response);
				
			fclose(page);
			close(connect_d);
			exit(0);
		}
		close(connect_d);		
	}
	return 0;
}

void error(char *msg)
{
	fprintf(stderr,"%s: %s\n",msg,strerror(errno));
	exit(1);
}

int open_listener_socket()
{
	int s = socket(PF_INET,SOCK_STREAM,0);  //Check what is SOCK_RAW
	if(s == -1)
		error("Can't Open Socket");
	return s;
}

void bind_to_port(int socket, int port)
{
	struct sockaddr_in name;
	name.sin_family = PF_INET;
	name.sin_port = (in_port_t)htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	int reuse = 1;
	if(setsockopt(socket,SOL_SOCKET,SO_REUSEADDR,(char *)&reuse,sizeof(int)) == -1)
		error("Cant set the reuse option on the socket.");
	int c = bind(socket,(struct sockaddr *)&name,sizeof(name));
	if(c == -1)
		error("Can't bind to socket.");
}

int say(int socket,char *s)
{
	int result = send(socket,s,strlen(s),0);
	if(result == -1)
		fprintf(stderr,"%s: %s\n","Error talking to the client",strerror(errno));
	return result;
}

int hear(int socket, char *buf, int len)
{
	char *s = buf;
	int slen = len;
	int c = recv(socket,s,slen,0);
	while((c>0) && (s[c-1] != '\n'))
	{
		s +=c;
		slen -= c;
		c = recv(socket,s,slen,0);
	}
	if(c<0)
		return c;
	else if(c==0)
		buf[0] = '\0';
	else
		s[c-1]='\0';
	return len - slen;
}

int catch_signal(int sig, void (*handler)(int))
{
	struct sigaction action;
	action.sa_handler = handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	return sigaction(sig,&action,NULL);
}

void handle_shutdown(int sig)
{
	if(listener_d)
		close(listener_d);
	fprintf(stderr,"Bye!!!\n");
	exit(0);
}
