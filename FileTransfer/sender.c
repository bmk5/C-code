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
#include <arpa/inet.h>
#include <time.h>

#include "lib.h"

int PORT = 9191;	  /* port we listen on*/
int CHUNKSIZE = 4096; /* size of our chunks*/
int BUFSIZE = 5015;	  /* this is the size of the buffer we send over the network*/
int totalChunks;	  /* these are the total number of chunks we are sending */
int num_blocks;		  /* these are the number of blocks each receiver gets*/
char **fileChunks;	  /* where we store the chunks of our files*/
int t_idx = 0;
int remReceivers;

void *sendToReceiver(void *vargp);

int main(int argc, char *argv[])
{
	// ensure user has passed enough arguments
	if (argc != 3)
	{
		fprintf(stderr, "usage: %s <no. of receivers> <filename>\n", argv[0]);
		exit(1);
	}

	struct sockaddr_in myaddr;	 /* my ip address info */
	struct sockaddr_in recvaddr; /* receiver ip address info */
	struct Args2 *args;			 /* holds the arguments we pass to a thread*/
	struct stat st;
	int sockfd;			/* socket file descriptor */
	int *recvSocket;	/* client socket file descriptor */
	socklen_t recv_len; /* byte size of client's address*/
	char *filename;		/* this is the file to be sent */
	int numReceivers;	/* this is the total number of receivers*/
	pthread_t tid;
	int optval;
	int size;

	// getting the command line arguments
	numReceivers = atoi(argv[1]);
	remReceivers = 0;
	filename = argv[2];

	double times[numReceivers];

	// getting the size  of the input file
	stat(filename, &st);
	size = st.st_size;
	printf("SIze of file:%d\n", size);

	// calculating the resulting chunks from the current chunk size
	totalChunks = (size / CHUNKSIZE);
	if (size % CHUNKSIZE > 0) totalChunks += 1;
	printf("Total number of file chunks: %d\n", totalChunks);

	num_blocks = (totalChunks / numReceivers);
	if (totalChunks % numReceivers > 0) num_blocks += 1;
	printf("Number of blocks for each receiver: %d\n", num_blocks);

	// initiating array that stores our fle chunks
	fileChunks = initArray(totalChunks, CHUNKSIZE);

	// splitting input file into chunks
	splitFile(fileChunks, filename);

	// create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		perror("socket creation failed");
		exit(1);
	}

	// binding the socket
	myaddr.sin_port = htons(PORT);
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server on same port immediately after we kill it;
	 * otherwise we have to wait about 20 secs.
	 * Eliminates "ERROR on binding: Address already in use" error.
	 * We also set the timeout of the client connection to 20 seconds
	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(int));

	// binding the socket to port 8181
	if (bind(sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0)
	{
		perror("binding the socket failed");
		exit(1);
	}

	// telling the OS to accept client connections
	if (listen(sockfd, 10) < 0)
	{
		perror("Listening activation failed");
		exit(1);
	}

	int idx = 0;
	/*
	 * We keep the socket open listening for client connections
	 * Once a client sends a request, we create a thread to serve the request
	 */
	while (remReceivers < numReceivers)
	{
		printf("Number of remaining receivers:%d\n", remReceivers);
		// accept incoming client connections
		recv_len = sizeof(recvaddr);
		recvSocket = (int *)malloc(sizeof(int));
		*recvSocket = accept(sockfd, (struct sockaddr *)&recvaddr, &recv_len);
		remReceivers++;
		printf("Receiver connected: ");
		printf("%s\n", inet_ntoa(recvaddr.sin_addr));

		// arguments we pass to thread
		args = (struct Args2 *)malloc(sizeof(struct Args2));
		args->receiver = recvSocket;
		args->index = idx;
		args->times = times;
		idx += num_blocks;

		/* try to create thread and run function "run_thread" */
		if (pthread_create(&tid, NULL, sendToReceiver, args) != 0)
		{
			printf("error creating threads\n");
			exit(1);
		}

		//   for (int i = 0; i < numReceivers; i++){
		//     printf("%f\t", times[i]);
		//   }
		printf("Remaining receivers: %d\n", remReceivers);
	}

	while (remReceivers > 0) continue;
	double average = 0;

	for (int i = 0; i < numReceivers; i++)
	{
		average += times[i];
	}

	double avg = average /numReceivers;
	printf("The average time taken for all %d receivers to receive the file '%s' is %f seconds\n", numReceivers, filename, avg);
	close(sockfd);
	return 0;
}

/*
 * This function takes in the receiver's socket and a file chunk
 * It sends the file chunk to the given receiver
 */
void *sendToReceiver(void *vargp)
{
	// extract our arguments from the struct
	int recvSocket = *(((struct Args2 *)vargp)->receiver);
	int idx = ((struct Args2 *)vargp)->index;
	double *times = ((struct Args2 *)vargp)->times;
	char hostName[10];
	char buf[BUFSIZE];
	time_t start_t, end_t;
	double diff_t;

	/* detach this thread from parent thread */
	if (pthread_detach(pthread_self()) != 0)
	{
		printf("error detaching\n");
		exit(1);
	}

	// represents the number of blocks we are to send to the receiver
	int stop = idx + num_blocks;

	// converting string to int
	char start[32];
	snprintf(start, sizeof(start), "%d", totalChunks);

	char end[5] = "done";

	printf("initializing chunk sending process\n");

	time(&start_t);

	// telling the receiver how many file chunks there are in total
	send(recvSocket, start, sizeof(start), 0);
	bzero(buf, BUFSIZE);
	
	// sending chunks to receiver
	for (int i = idx; i < stop && i < totalChunks; i++)
	{
		/* starting with the chunk id then adding the file chunk*/
		snprintf(buf, sizeof(buf), "%d", i);
		int offset = strlen(buf);
		memcpy(buf+offset, ":\n",2);
		offset+=2;		
		memcpy(buf+offset, fileChunks[i],CHUNKSIZE);
	
		
		//printf("\nBUFFER BEING SENT:\n%s\n", buf);
		send(recvSocket, buf, BUFSIZE, 0);
		bzero(buf, BUFSIZE);	

		// wait for receiver to acknoweldge receipt of the block
		recv(recvSocket,start,sizeof(start),0);
		if (strcmp(start,"done") == 0){
			printf("\nRECEIVER HAS CONFIRMED RECEIPT OF FILE CHUNK %d\n",i);
		}	
	}

	// signal to receiver that we are done sending this buffer
	send(recvSocket, end, sizeof(end), 0);
    
	// wait for receiver to signal that they are done receiving all blocks of the file
	recv(recvSocket, hostName, sizeof(hostName), 0);
	printf("\nReceived hostname: %s\n", hostName);
    
	// record the time taken and store it into an array
	time(&end_t);
	remReceivers--;
	diff_t = difftime(end_t, start_t);
	printf("\nTime taken in seconds:%f\n", diff_t);
	times[t_idx++] = diff_t;

	if (vargp) free(vargp);
	close(recvSocket);
	return NULL;
}
