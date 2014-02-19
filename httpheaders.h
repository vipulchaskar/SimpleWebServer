//a quick library to parse the essential HTTP Request headers

struct httpheaders
{
  int method;     //0 for GET, 1 for POST
  char path[256];
  int valid_http; //valid_http = 1 if using HTTP/1.1
  char host[20];
};

struct httpheaders parsehttp(char buffer[])
{
  struct httpheaders headers;
  char method[5];
  char *hoststart;
  int i,j;
  for(i=0;buffer[i]!=' ';i++)
    method[i] = buffer[i];
  method[i] = '\0';
  for(i++,j=0;buffer[i]!=' ';i++,j++)
    headers.path[j] = buffer[i];
  headers.path[j] = '\0';
  hoststart = strstr(buffer,"Host:");
  for(hoststart=hoststart+6,i=0;*hoststart!='\n';hoststart++,i++)
    headers.host[i] = *hoststart;
  headers.host[i]='\0';
  if(strcmp(method,"GET")==0)
    headers.method = 0;
  else
    headers.method = 1;
  if(strstr(buffer,"HTTP/1.1"))
    headers.valid_http = 1;
  else
    headers.valid_http = 0;
  return headers;
}

