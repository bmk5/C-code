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

#include "lib.h"

char *SENDER = "137.165.8.10"; /* ip address of the sender*/
int PORT = 9191;               /* port we listen and connect to*/
int BUFSIZE = 5015;            /* this is the size of the buffer we send over the network*/
int CHUNKSIZE = 4096;          /* size of our chunks*/
int MINBUF = 32;               /* size of a small message buffer*/
int idx = 0;                   /* keeps track of the connections we currently have*/
int counter = 0;               /* keeps track of the total chunks received*/
int numBlocks;                 /* number of blocks receiver has */
int totalBlocks;               /* total number of file chunks there are */
int remBlocks;                 /* out of the total number of blocks, how many remain*/
char **recvs;                  /* array containing ip addresses of other receivers*/
char **buffers;                /* stores block of data we receive from sender */
char **blocks;                 /* stores the file chunks in order of their chunk ids*/
char **connections;            /* this stores the IP addresses of current connections on a receiver*/
pthread_mutex_t lock;

void *receiveBlock(void *vargp);
void *runConnect(void *vargp);
void *sender(void *vargp);

/*
 * This function connects this receiver to the sender
 * It connects to the sender to receive a file chunk
 */
int connectToSender(int numReceivers)
{
    int status;
    int sock_fd; /* this is the socket we use to connect to the sender*/
    struct sockaddr_in serv_addr;

    // printf("creating connection\n");
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // here we are converting the IP address of the sender into binary form
    if (inet_pton(AF_INET, SENDER, &serv_addr.sin_addr) < 1)
    {
        printf("Failed to convert \n");
        return -1;
    }

    // connecting to the host now
    status = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (status < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    printf("\nConnected to Sender\n");

    char temp[MINBUF];
    bzero(temp,MINBUF);

    // getting the number of file chunks there are in total
    recv(sock_fd, temp, sizeof(temp), 0);
    totalBlocks = atoi(temp);
    // printf("totalBlocks: %d\n", totalBlocks);
    remBlocks = totalBlocks;
    numBlocks = (totalBlocks / numReceivers);
    if (totalBlocks % numReceivers > 0) numBlocks +=1;

    // initializing array that holds the buffers that were sent
    buffers = initArray(numBlocks, BUFSIZE);

    // initializing array that holds the actual file chunks
    blocks = initArray(totalBlocks, CHUNKSIZE);

    int k = 0;
    
    printf("Receiving %d blocks\n",numBlocks);
    // keep receiving until sender completes sending all the chunks
    for (int i = 0; i < numBlocks; i++)
    {
        int numRead = 0;
        
        //printf("Now processing data at buffer %d\n", i);
        char temp[BUFSIZE];
        bzero(temp,BUFSIZE);
        int bytes = 0;
       
        /* here we read in data until the whole buffer is read in*/
        while (bytes < BUFSIZE){
            numRead = recv(sock_fd, temp, BUFSIZE, 0);

            if (numRead <= 0){
                perror("Problem in receiving data\n");
                exit(1);
            }
            if (strcmp(temp, "done") == 0) break; // sender is done sending this buffer
            
            // copy the buffer into our array of buffers
            memcpy(buffers[i]+bytes,temp,numRead);
            //printf("\nCopied our buffer into buffer array successfully:\n%s\n\n",temp);
            bzero(temp,BUFSIZE);
            bytes+= numRead;
        }

        if (strcmp(temp, "done") == 0) break;
        //printf("-----------------------------OUR BUFFER:--------------------------\n%s\n", buffers[i]);
        
        /* signal to the sender that we have received the buffer*/
        char end[5] = "done";
        send(sock_fd,end,5,0);

        char index[MINBUF];
        bzero(index,MINBUF);

        /*parsing our buffer to get the chunk id
         * buffer is of the form "<chunkid>:\n<data from file>"
         */
        char *p = strstr(buffers[i], ":\n");
        if (p == NULL)
        {
            printf("Did not find a colon!!!\n\n");
            continue;
        }
        memcpy(index, buffers[i], (p - buffers[i]));
        
        int blockNum = atoi(index);
        //printf("About to copy file chunk into our array at index %d\n\n",blockNum);

        /* copy the file chunk into our array of file chunks IN ORDER*/
        memcpy(blocks[blockNum], p + 2, CHUNKSIZE);
        pthread_mutex_lock(&lock);
        remBlocks--;
        pthread_mutex_unlock(&lock);
        
        k++;
    }

    // sometimes the sender can send less blocks than expected
    numBlocks = k;

    // we return the socket to be used later to alert the sender that we are done
    return sock_fd;
}

/*
 * This function is one called by a thread
 * It creates a socket on which this receiver listens for other receivers
 * It accepts a connection from another receiver sending their file chunk
 */
void *listenForReceivers(void *vargp)
{
    struct sockaddr_in myaddr;     /* my ip address info */
    struct sockaddr_in clientaddr; /* client ip address info */
    pthread_t tid;                 /* thread id*/
    int sockfd;                    /* this is the socket we listen on */
    int *recvSocket;               /* receiver socket file descriptor */
    int optval;
    socklen_t recv_len; /* byte size of receiver's address*/

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

    while (1)
    {
        // accept incoming client connections
        recv_len = sizeof(clientaddr);
        recvSocket = (int *)malloc(sizeof(int));
        *recvSocket = accept(sockfd, (struct sockaddr *)&clientaddr, &recv_len);
        printf("\n\nReceiver connected.\n");

        // getting the ip address of the receiver connecting to us
        char *recvIp = inet_ntoa(clientaddr.sin_addr);

        // locking the connections array to write to it then unlocking
        pthread_mutex_lock(&lock);
        int st = addToConnections(recvIp, idx, connections);
        pthread_mutex_unlock(&lock);

        // if this is a new connection(one not in the array) increment the index
        if (st == 0)
            idx++;

        if (*recvSocket < 0)
        {
            printf("ERROR on accept\n");
            exit(1);
        }

        // these are the arguments we are passing to the thread, we store them in the struct
        struct Args *args = (struct Args *)malloc(sizeof(struct Args));
        args->receiver = recvSocket;
        strcpy(args->chunk, recvIp);

        /* try to create thread and run function "run_thread" */
        if (pthread_create(&tid, NULL, receiveBlock, args) != 0)
        {
            printf("error creating threads\n");
            exit(1);
        }
    }

    close(sockfd);
    return NULL;
}

/*
 * This function receives a file chunk from a receiver and writes the block to disk
 */
void *receiveBlock(void *vargp)
{
    char buf[BUFSIZE];                                /* message buffer */
    int recvfd = *(((struct Args *)vargp)->receiver); /* this is the socket on which we receive the file chunk*/
    char *ip = ((struct Args *)vargp)->chunk;         /* this is the IP address of the other receiver*/

    //printf("Now receiving block from another receiver with Ip: %s\n", ip);

    char temp[MINBUF];
    bzero(temp, MINBUF);

    // getting number of blocks to expect from other receiver
    recv(recvfd, temp, sizeof(temp), 0);
    int num = atoi(temp);
    
    if (num == 0) return NULL;
    //printf("Number of blocks to expect:%d\n", num);

    // wait until other receiver sends all blocks
    for (int i = 0; i < num; i++){ 
        bzero(buf,BUFSIZE);
        int numRead = 0;
        char temp[BUFSIZE];
        bzero(temp,BUFSIZE);
        int bytes = 0;
        
        /* keep receiving till we have received the full buffer*/
        while (bytes < BUFSIZE){
            numRead = recv(recvfd, temp, BUFSIZE, 0);
            if (numRead <= 0){
                perror("Problem in receiving data\n");
                exit(1);
            }
            
            // copy received buffer into our buffer
            memcpy(buf+bytes,temp,numRead);
            printf("Copied our buffer into buffer array successfully\n");
            bzero(temp,BUFSIZE);
            bytes+= numRead;
        }
        
        send(recvfd,"done",5,0);

        //printf("--------------------------------------Received buffer---------------------------%s\n", buf);

        
        printf("Now storing my received data block\n");

        char index[MINBUF];
        bzero(index, MINBUF);

        // obtaining the chunk id from the buffer
        char *p = strstr(buf, ":\n");
        memcpy(index, buf, (p - buf));
        int blockNum = atoi(index);
        printf("The block index is %d\n", blockNum);

        memcpy(blocks[blockNum], p + 2, CHUNKSIZE);
        pthread_mutex_lock(&lock);
        remBlocks--;
        pthread_mutex_unlock(&lock);
    
    }

    free(vargp);
    return NULL;
}

/*
 * This function sends the file chunks currently held by this receiver to all
 * other receivers on the network
 */
void sendBlocks()
{
    printf("\n-------------------------Sending blocks to receivers I know--------------------\n");
    printf("Connections available:%d\n", idx);

    /* we iterate through the current connections we have in the connections array
     * and send our file chunks to each of those receivers
     */
    for (int i = 0; i < idx; i++)
    {
        int sock_fd; /* this is the socket on which we send our file chunks*/
        int status;
        struct sockaddr_in serv_addr;
        char *ip = connections[i]; /* ip address of receiver*/

        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        /*convert IP address to binary representation*/
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported for %s\n", ip);
            return;
        }

        status = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (status < 0)
        {
            perror("connection failed");
            return;
        }

        char start[MINBUF];
        bzero(start, MINBUF);

        snprintf(start, 32, "%d", numBlocks);

        printf("Sending %s buffers now to :%s\n", start, ip);

        // telling other receiver how many blocks to expect
        send(sock_fd, start, sizeof(start), 0);

        // send all blocks to other receiver
        for (int i = 0; i < numBlocks; i++)
        {
            //printf("------------------------BUFFER BEING SENT:----------------------------\n%s\n", buffers[i]);
            int num_sent = send(sock_fd, buffers[i], BUFSIZE, 0);
           printf("Number of bytes sent:%d\n", num_sent);
            
            // wait for receiver to confirm receipt of the block
            recv(sock_fd,start,sizeof(start),0);
            if (strcmp(start,"done")==0){
                //printf("\nRECEIVER HAS ACKNOWLEDGED RECEIPT OF BLOCK %d\n",i);
            }
        }

        printf("--------------------------------------------------------------------\n");
        close(sock_fd);
    }
}

/*
 * This function tries to connect to all the receivers on our network
 * If we successfully connect, then we add the receiver to our list of connections
 * We DO NOT send any file chunk when we connect
 */
void connectToReceivers(int numRecs)
{
    //printf("\n--------------------Trying to connect to available receivers-------------------\n");

    /* Here we iterate through the recvs array that holds all the receivers in the network
     *  We try and connect to each one of them
     */
    for (int i = 0; i < numRecs - 1; i++)
    {
        int status;
        int sock_fd; /* socket on which we try and connect*/
        struct sockaddr_in serv_addr;

        // get the IP address of the receiver and remove trailing newlines
        char *ip = recvs[i];
        ip[strcspn(ip, "\n")] = '\0';
        ip[strcspn(ip, "\r")] = '\0';

        if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error \n");
            return;
        }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);

        // convert string IP address to binary
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported for %s\n", ip);
            return;
        }

        status = connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        if (status < 0)
        {
            perror("Connection to a receiver failed");
            return;
        }

        //printf("Connected to a receiver\n");

        // add the connection to our connections array. We lock it first because of threading
        pthread_mutex_lock(&lock);
        int st = addToConnections(ip, idx, connections);
        pthread_mutex_unlock(&lock);

        if (st == 0)
            idx++; // if this is a new connection, increment our counter
        //printf("--------------------------------------------------------------\n");
    }
}

int main(int argc, char const *argv[])
{
    // ensure user has passed enough arguments
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <no. of receivers>\n", argv[0]);
        exit(1);
    }

    int numReceivers;             /* number of total receivers we are dealing with*/
    pthread_t listenRecv;         /* thread we create to listen for receivers*/
    int sender_fd;                /* the socket for the sender */
    numReceivers = atoi(argv[1]); /* total number of receivers on our network*/
    char hostbuffer[64];          /* holds the name of this receiver*/

    connections = initArray(numReceivers - 1, 20); /* initiating our connections array*/
    recvs = initArray(numReceivers - 1, 20);       /* initiating our receivers array*/
    getReceivers(recvs, numReceivers - 1);         /* getting all the receivers on our network*/
    gethostname(hostbuffer, sizeof(hostbuffer));

    // initiating thread lock
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init has failed\n");
        exit(1);
    }

    sender_fd = connectToSender(numReceivers);

    /* creating a thread to listen for receivers*/
    if (pthread_create(&listenRecv, NULL, listenForReceivers, NULL) != 0)
    {
        printf("error creating threads\n");
        exit(1);
    }

    printf("Receivers:");
    for (int i = 0; i < (numReceivers - 1); i++)
    {
        printf("%s\t", recvs[i]);
    }
    printf("\n");

    /* try and connect to all receivers*/
    connectToReceivers(numReceivers);

    /* connect to sender to receive block of text*/

    printf("current connections:%d\n", idx);

    /* We wait until we are connected to every receiver on the network
     *  before trying to connect to each one of them
     */
    while (idx < (numReceivers - 1)) continue;
    printf("\n---------------------Now looping through connections------------------------\n");

    /* send our file chunks to every receiver*/
    if (numReceivers > 1) sendBlocks();

    /* wait until we are done listening for other receivers
     * to send their file chunks
     */
    while (remBlocks > 0){
        //printf("I haven't received all my blocks for some reason!!\n"); 
        continue;
    }
    send(sender_fd, hostbuffer, sizeof(hostbuffer), 0);
    printf("Now writing my %d blocks to file\n", totalBlocks);

   
    // write our blocks to disk
    writeToFile(blocks, totalBlocks);

    // free allocated arrays
    freeArray(recvs, numReceivers - 1);
    freeArray(connections, numReceivers - 1);
    freeArray(buffers, numBlocks);
    freeArray(blocks, totalBlocks);

    return 0;
}
