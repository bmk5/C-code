/*
* This file contains the code that runs the server, by building and binding the socket
* It also listens and accepts requests from the client, serves those requests by returning the
* appropriate response to the client.
*/

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>


#include "lib.h"  // contains definitions of errors, ports and servers


int serve_request(char **res, char *root, int clientSocket, int host_exists);
int isValidRequest(char *request, char *protocol,int host_exists);
void set_header(int content_len,int status_code, int clientSocket,char *http_type, char *file_type);
int check_filepm(char *filepath);
char* checkFileType(char *file);
void zeroArray(char **arr);


void *run_thread(void *vargp);

extern int errno;

int main(int argc, char *argv[])
{
    struct sockaddr_in myaddr;    /* my ip address info */
    struct sockaddr clientaddr;   /* client ip address info */
    struct Args *args;            /* holds the arguments we pass to a thread*/
    pthread_t tid;                /* thread id*/
    int sockfd;                   /* socket file descriptor */
    int port;                     /* port on which we listen for connections*/
    int *clientSocket;            /* client socket file descriptor */
    int optval;
    socklen_t client_len;         /* byte size of client's address*/
    
    
    
    
    
    //ensure user has passed enough arguments
    if (argc != 3){
      fprintf(stderr, "usage: %s <root directory> <port number>\n",argv[0]);
      exit(1);
    }
    
   
    //create socket
    sockfd = socket(AF_INET,SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    //binding the socket
    port = atoi(argv[2]);
    myaddr.sin_port = htons(port);
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* setsockopt: Handy debugging trick that lets 
   * us rerun the server on same port immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   * We also set the timeout of the client connection to 20 seconds
   */
    optval = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR, (const char *) &optval, sizeof(int));

    
    // binding the socket to port 8181
    if(bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr))<0) {
        perror("binding the socket failed");
        exit(1);
    }

    // telling the OS to accept client connections
    if (listen(sockfd, 10) < 0) {
        perror("Listening activation failed");
        exit(1);
    }

    /*
    * We keep the socket open listening for client connections
    * Once a client sends a request, we create a thread to serve the request
    */ 
    while (1){
      // accept incoming client connections
      client_len = sizeof(clientaddr);
      clientSocket = (int *) malloc(sizeof(int));
      *clientSocket = accept(sockfd,(struct sockaddr*) &clientaddr, &client_len);
      printf("Client connected.\n");

      struct timeval tv;
      tv.tv_sec = 20;
      tv.tv_usec = 0;
      setsockopt(*clientSocket,SOL_SOCKET,SO_RCVTIMEO, (const char *) &tv, sizeof(tv));
      
      // these are the arguments we are passing to the thread, we store them in the struct
      args = (struct Args*) malloc(sizeof(struct Args));
      args->client = clientSocket;
      args->path = argv[1];
      
       /* try to create thread and run function "run_thread" */
      if (pthread_create(&tid,NULL,run_thread,args)!=0){
        printf("error creating threads\n");
	      exit(1);
      }
    }
  
  close(sockfd);
  return 0;
}

/*
* This method runs a single thread that serves a client's request
*/
void *run_thread(void *vargp){
 
 //extract our arguments from the struct
 int clientSocket = *(((struct Args *)vargp)->client);
 char *root = ((struct Args *) vargp)->path;
 
 char buffer[SIZE] = {0};  /* buffer to store incoming client requests*/
 int close_conn = 2;       /* indicates whether we close the connection.*/
  
 /* detach this thread from parent thread */
 if (pthread_detach(pthread_self())!=0){
   printf("error detaching\n");
   exit(1);
 }
 
 /* free heap space for vargp since we have our two arguments */
 if (vargp) free(vargp);

  /*
  * We keep the connection open as long as the protocol is HTTP/1.1
  * "close_conn" tells us what connection type it is
  * 2 represents HTTP/1.1, 1 represents HTTP/1.0
  */
  while(close_conn == 2){ 
	  int flag = recv(clientSocket, buffer, SIZE,0);
    
    if (flag < 0){
      /* if our timeout value of 20 seconds set in setockopt is passed,
      *  we terminate the connection
      */
      if (errno == ETIMEDOUT) printf("Connection timed out\n");
      close(clientSocket);
      break;
    }
    /* recv returns 0 when client closes connection smoothly*/
    if (flag == 0){
       printf("Client has terminated connection\n");
       close(clientSocket);
       break;
    }
    	
    /*
    * this array holds three pointers that point to three strings
    * the first pointer will point to what should be the "GET" command
    * the second pointer will point to what should be the filename
    * the third pointer will point to what should be the protocol
    */
    char **res = initArray();
    char *ptr;

      /*
      * we iterate through our request buffer to make sure we process
      * any "GET" request within the server
      */ 
      while((ptr = parse_buffer(buffer)) && close_conn > 1){
        char temp[SIZE] = {0};      /*serves as a temporary buffer*/
        
        /* we get the size of the remaining buffer after we get rid of the 
        *  first "GET" request in the server. Then we copy the contents of the
        *  remaining buffer into our temporary buffer. 
        */
        int rem = (buffer + sizeof(buffer)) - (ptr+2);
        memcpy(temp,ptr+2,rem);

        /* we parse a single "GET" request and set the values of our array*/
        int host_exist = parse_request(buffer,res);
        
        /* we then serve the "GET" command we have obtained*/
        close_conn = serve_request(res,root,clientSocket,host_exist);

        /*close the connection if the protocol was HTTP/1.0*/
        if (close_conn == 1) close(clientSocket);
        
        /*Set our buffer to be the remaining requests*/
        memcpy(buffer,temp,SIZE);
      }
   
   /*freeing our malloced array*/
   freeArray(res);
  } 
 
 close(clientSocket);
 return NULL;
}

/*
* This method takes in a single request of the form " GET <filename> <protocol>"
* It checks to see whether the request is valid, that the file exists and that
* the user has the permissions to access that file. If all is well, it returns
* the requested resource. It returns 1 if the protocol used was HTTP/1.0, otherwise
* it returns 2.
*/
int serve_request(char **res, char *path,int clientSocket,int host_exists){

  struct stat st;                       /* will contain the details of the file we want to open*/
  char buf[SIZE] = { 0 };               /* Buffer that contains the response for the client*/
  char default_protocol[]="HTTP /1.1";  /* default protocol in case of error in the request*/
  char default_type[]="text/html";      /* default type we return would be a html page*/
  char filepath[200] = {0};            /* absolute file path to the requested resource*/
  char *command;                        /* should essentially be "GET"*/
  char *file;                           /* the name of the requested resource,e.g, file name*/
  char *protocol;                       /* protocol the client is using*/
  char *file_type;                      /* returns the MIME type of the file*/  
  int fd;                               /* file descriptor of file we want to open*/
  int content_len;                      /* length of the server response*/
  int close_conn;                       /* indicator of the protocol type*/
  int valid_request;                    /* indicates whether request is valid or not*/
  
  /*we perform a deep copy so as to avoid tampering with the
  * original file path in the future in case it is needed
  */
  memcpy(filepath,path,strlen(path));

  /* extracting our request parameters from the malloced array*/
  command = *res;
  protocol = *(res+2);
  
  /* removing trailing newline from protocol type */
  protocol[strcspn(protocol,"\r\n")] = 0;

  /* deal with the case where the file name is "/"*/
  if (strcmp(*(res+1), "/") == 0) file = "/index.html"; 
  else file = res[1];

  /* checking if the client request is valid */
  valid_request = isValidRequest(command,protocol,host_exists); 
  if (valid_request < 0){ //request has invalid message framing
    set_header(strlen(INVALID_REQUEST),400,clientSocket,default_protocol,default_type);
    send(clientSocket,INVALID_REQUEST,strlen(INVALID_REQUEST),0);
    return 2;
  }
  else if (valid_request > 0) close_conn = valid_request;
  
  /* we check what MIME type the file is*/
  file_type = checkFileType(file);

  /* constructing absolute file path to requested file */
  strcat(filepath,file);
  
  /* check permissions for the file*/
  int pm = check_filepm(filepath);
  if (pm < 0){ //file does no exist error
    set_header(strlen(NOT_FOUND),404,clientSocket,protocol,default_type);
    send(clientSocket,NOT_FOUND,strlen(NOT_FOUND),0);
    return close_conn;
  }

  else if (pm > 0){//file is not world readable
    set_header(strlen(FORBIDDEN),403,clientSocket,protocol,default_type);
    send(clientSocket,FORBIDDEN,strlen(FORBIDDEN),0);
    return close_conn;
  }
  
  //opening the file
  fd = open(filepath, O_RDONLY);
  stat(filepath,&st);  
  content_len = st.st_size;
  
  /* set the headers for the response */
  set_header(content_len,200,clientSocket,protocol,file_type);
  
  /* read the file and send the contents to the client*/
  int nread;
  while ( (nread = read(fd,buf,sizeof(buf))) > 0){
     send(clientSocket,buf,sizeof(buf),0);
  }

  close(fd);
  return close_conn;
}

/*
* This method takes in a file name along with its extension as a parameter
* It returns the MIME equivalent of the given file
*/
char* checkFileType(char *file){
  char type[10]={0};

  //finding the position of the '.' in the text then getting the substring from there
  int pos = strcspn(file,".") + 1;
  int size = strlen(file) - pos;
  memcpy(type,file+pos,size);
  
  if (strcmp(type,"html") == 0) return "text/html";
  else if (strcmp(type,"jpg") == 0) return "image/jpeg";
  else if (strcmp(type,"pdf") == 0) return "application/pdf";
  else if (strcmp(type,"gif") == 0) return "image/gif";
  return "";
}

/*
* This method takes in the command and protocol within the client's request
* and checks to see whether it is valid. It also takes in a flag that indicates
* whether the "Host" line was present in the client's request.
* It returns -1 if the request is invalid, otherwise 0
*/
int isValidRequest(char *command, char *protocol,int host_exists){
  if (strcmp(command,"GET") != 0 ) return -1;
  

  if ((strcmp(protocol,"HTTP/1.0") == 0)) return 1;

  else if ((strcmp(protocol,"HTTP/1.1") == 0)) {
    /* if the protocol is HTTP/1.1 and the host field was absent
    * then it is an invalid request
    */
     if (host_exists < 1) return -1;
     return 2;
  }

  else return -1;
  

  return 0;
}

/*
* This method takes in a filename and checks to see whether
* the client has permission to access the file.
* Returns -1 if file does not exist, 0 if client has permissions
* to check the file, 1 if the client does not have permissions
*/
int check_filepm(char *filename){
  struct stat fs;
  int r;  
  char *p;

  p = strstr(filename,"..");
  r = stat(filename,&fs);

  if (p) return 1;
  
  
  /* file does not exist error */
  if (r == -1) return -1;

  //bitwise and to check world permissions
  if (fs.st_mode & S_IROTH) return 0;
  return 1;
}

/*
* This method essentially constructs the header for the server response
* with the given parameters passed to the function and sends the newly
* constructed header to the client.
*/
void set_header(int content_len,int status_code, int clientSocket,char *http_type, char *file_type){
  char *conLength;            /* "Content-Length" header */
  char *conType;              /* "Content-Type" header */
  char *status;               /* Status code header */
  char header[SIZE];

  conLength = "Content-Length:";
  conType = "Content-Type: ";
   
  if (status_code == 200) status = "200 OK";
  else if (status_code == 404) status = "404 Not Found";
  else if (status_code == 400) status = "400 Bad Request";
  else if (status_code == 403) status = "403 Forbidden";
  
  /* construct the header */
  snprintf(header,sizeof(header),
           "%s %s\n%s\n%s\n%s %d\n%s%s\r\n\r\n",http_type,status,DATE,SERVER,conLength,content_len,conType,file_type);
  
  /* send to the client */
  send(clientSocket,header,strlen(header),0);
}
