#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"
int BUF = 4096; /* size of our chunks*/

/*
 * This functions takes in an empty array of strings and an integer 'num' representing the expected number of receivers
 * on the network.
 * It then reads in a file containing all receivers on the networks and only reads in 'num' receivers
 */
void getReceivers(char **recvs, int num)
{
	// obtain and store hostname of this receiver
	char hostbuffer[64];
	gethostname(hostbuffer, sizeof(hostbuffer));

	FILE *ptr;
	char line[256];

	// Opening file in reading mode
	ptr = fopen("addresses.txt", "r");
	int k = 0;

	/*
	 * We read the file line by line. Each line is of the form: <hostname>:<IP Address>
	 * We skip over ourselves and only read in the given number of receivers
	 */
	while (fgets(line, sizeof(line), ptr) && num > 0)
	{
		// obtaining the hostname from the line
		char name[16] = {0};
		char *p = strchr(line, ':') + 1;
		memcpy(name, line, (p - line - 1));

		// skip over our own hostname
		if (strcmp(hostbuffer, name) == 0)
			continue;
		num--;

		// add the IP address to our list of receivers
		strcpy(recvs[k++], p);
	}

	fclose(ptr);
}

/*
 * This function takes in a file path, the number of receivers on the network and an empty array
 * It reads in the file pointed to by the path, splits it into chunks, and stores the chunks into the array
 */
void splitFile(char **chunks, char *filename)
{
	int fd;
	char buf[BUF];

	// Opening file in reading mode
	fd = open(filename, O_RDONLY);
	int k = 0;

	// read the file chunk by chunk and store it in the chunks array
	while (read(fd, buf, BUF) > 0)
	{
		bzero(chunks[k], BUF);
		memcpy(chunks[k++], buf, BUF);
		memset(buf, 0, BUF);
	}

	close(fd);
}

/*
 * This method allocates space for an array of size 'num' where each entry is a string
 * of at most size 'size'. It returns the allocated array
 */
char **initArray(int num, int size)
{
	char **arr = (char **)malloc(num * sizeof(char *));

	for (int i = 0; i < num; i++)
	{
		arr[i] = (char *)malloc(size * sizeof(char));
		// printf("block pointer %d: %x\t", i, arr[i]);
		bzero(arr[i], size);
	}
	// printf("\n\n");

	return arr;
}

/*
 * This funtion deallocates a previously allocated array 'arr' of size 'num'
 */
void freeArray(char **arr, int num)
{
	for (int i = 0; i < num; i++)
	{
		if (arr[i]) free(arr[i]);
	}
	if (arr)
		free(arr);
}

/*
 * This function takes in an array 'connections' containing IP Addresses, an index and an IP Address.
 * It adds the given IP Address to connections if it is not contained in connections
 */
int addToConnections(char *ip, int idx, char **connections)
{
	printf("Adding connection at index:%d\n", idx);

	for (int i = 0; i <= idx; i++)
	{
		char *currIp = connections[i];
		printf("Position in array:%d \t\t IP in array:-%s- \t\t IP we want to add:-%s-\n\n", i, currIp, ip);
		if (strcmp(currIp, ip) == 0) return -1; // checking if IP is already in array
	}

	// adding IP Address to array
	strcpy(connections[idx], ip);
	printf("Copied address:%s\n", connections[idx]);
	return 0;
}

/*
 * This function takes in a buffer and writes it to disk
 */
void writeToFile(char **blocks, int totalBlocks)
{
	char hostbuffer[20];
	gethostname(hostbuffer, sizeof(hostbuffer));

	FILE *fPtr;
    
	// creating the file path
	char path[256] = "/home/cs-students/23bmk5/Documents/CS339/finalProject/";
	char *file = "/result.txt";

	strcat(path, hostbuffer);
	strcat(path, file);


	/* Open file in write append mode */
	fPtr = fopen(path, "ab");

	/* fopen() return NULL if last operation was unsuccessful */
	if (fPtr == NULL)
	{
		/* File not created hence exit */
		printf("Unable to create file.\n");
		exit(EXIT_FAILURE);
	}

	/* Write data to file */
	for (int i = 0; i < totalBlocks; i++)
	{
		fputs(blocks[i],fPtr);
	}
	printf("Wrote blocks to file\n");

	/* Close file to save file data */
	fclose(fPtr);
}
