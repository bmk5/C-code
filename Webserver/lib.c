#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"

/* 404 Not found error*/
char *NOT_FOUND =
"<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>The requested URL was not found on this server</h1>\n<hr>\n</body></html>\n\n";

/* 400 Invalid request error */
char *INVALID_REQUEST =
"<html><head>\n<title>400 Bad Request</title>\n</head><body>\n<h1>The server  URL cannot process the request due to invalid request message framing</h1>\n<hr>\n</body></html>\n\n";

/* 403 Forbidden error */
char *FORBIDDEN =
"<html><head>\n<title>403 Bad Forbidden</title>\n</head><body>\n<h1>The client does not have access rights to the requested resource</h1>\n<hr>\n</body></html>\n\n";



/*
* This method takes in a string that contains one or more requests from a client
* It looks for the first consecutive new lines and returns a pointer to the position
* of them. If no such newlines exists it returns NULL.
*/
char* parse_buffer(char *buffer){
  char *p1;
  char *p2;
  char *p3;
  
  /* look for the first occurrence of double newlines */
  p1 = strstr(buffer,"\r\n\r\n");
  p2 = strstr(buffer,"\n\n\n\n");
  
  /* return whichever form was found*/
  if(p1 || p2){
    if (p1) return p1+3;
    else return p2+3;
  }
  
  /* In the case of telnet we look for just one newline */
  p3 = strchr(buffer,'\n');
  if (p3) return p3+1;
  
  return NULL;
  
}

/*
* This method takes in a single "GET" request from the user,
* parses it, populates our array accordingly.
* It returns 1 if the "Host" line exists in the request,
* otherwise it returns zero
*/
int parse_request( char * buffer, char ** res){
  int host_exist = 0;  /* checks whether host exists*/

  //first, check if host exists 
  char *p1;
  char hostline[10] = {0};
  
  /* find position of first newline then check
  * whether subsequent line is the host line
  */
  p1 = strstr(buffer,"\r\n");   
  memcpy(hostline,p1+2,4);
  
  if (strcmp(hostline,"Host") == 0) host_exist = 1;
   
  /* We now parse the first line of the request to
  * obtain the parameters and populate our array
  */
  char *p = strtok(buffer," ");
  int i = 0;

  while ( p!= NULL && i < 3){ // we only need 3 parameters for our array
    strcpy(res[i++],p);
    p = strtok(NULL, " ");
  }

  return host_exist;
}

/*
* This method allocates space for the array that holds our
* request parameters and returns a pointer to that array
*/
char **initArray(){
  char **arr = (char **) malloc(3 * sizeof(char *));;
  arr[0] = (char *)malloc(10 * sizeof(char));
  arr[1] = (char *)malloc(20 * sizeof(char));
  arr[2] = (char *)malloc(8 * sizeof(char));

  return arr;
}

/*
* Frees memory allocated to the above array
*/
void freeArray(char ** arr){
  if (arr[0]) free(arr[0]);
  if (arr[1]) free(arr[1]);
  if (arr[2]) free(arr[2]);
  if (arr) free(arr);  
}

