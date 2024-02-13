/*
* This file holds definitions for various variables, HTTP error codes
* and a few helper functions
 */

#define SIZE 4096
#define SERVER "Server: Apache/2.4.41 (Ubuntu)"
#define DATE "Date : Sat, 11 Feb 2023 03:54:08 GMT"

/*defining http status codes */
extern char *NOT_FOUND;
extern char *INVALID_REQUEST;
extern char *FORBIDDEN;

/* struct that holds arguments we need to pass to a thread*/
struct Args 
{
  int *client;
  char *path;
};


char* parse_buffer(char *buffer);
int parse_request(char * buffer, char ** res);
char **initArray();
void freeArray(char **res);