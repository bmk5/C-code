struct Args 
{
  int *receiver;
  char chunk[2048];
};

struct Args2 
{
  int *receiver;
  int index;
  double *times;
};

void freeArray(char ** arr, int num);
void writeToFile(char **blocks, int totalBlocks);
void getReceivers(char ** recvs, int num);
char **initArray(int num, int size);
void splitFile(char ** chunks, char *filename);
int addToConnections(char *ip,int idx, char **connections);
int parseBuffer(char *buffer, char *ptr);