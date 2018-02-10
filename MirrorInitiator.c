/* inet_str_client.c: Internet stream sockets client */
#include <stdio.h>
#include <sys/types.h>	     /* sockets */
#include <sys/socket.h>	     /* sockets */
#include <netinet/in.h>	     /* internet sockets */
#include <unistd.h>          /* read, write, close */
#include <netdb.h>	         /* gethostbyaddr */
#include <stdlib.h>	         /* exit */
#include <string.h>	         /* strlen */
#include <getopt.h>
void perror_exit(char *message);

int main(int argc, char *argv[]) {
    int bytes_read,   port, sock, i;
    int size,MirrorServerPort,option;
    char * requests;
    char *MirrorServerAddress;
    char            buf[256];
    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*)&server;
    struct hostent *rem;
    static struct option long_options[] = {
        {"n",required_argument,NULL  ,  'q' },
        {"p",required_argument,NULL,  'x' },
        {"s",required_argument, NULL,  'y' },
        {NULL,0,NULL, 0}
    };

    while ((option = getopt_long_only(argc, argv,"q:x:y:",long_options,NULL)) != -1)
    {
        switch (option) 
        {
             case 'q' : size=strlen(optarg)+1;
                        MirrorServerAddress=malloc(sizeof(char)*size);
                        strcpy(MirrorServerAddress,optarg);
                 break;
             case 'x' : MirrorServerPort=atoi(optarg);
                 break;
             case 'y' : size=strlen(optarg)+1;
                        requests=malloc(sizeof(char)*size);
                        strcpy(requests,optarg);
                 break;
             default: 
                 printf("must be ./MirrorInitiator -n <MirrorServerAddress> -p <MirrorServerPort> -s <ContentServerAddress1:ContentServerPort1:dirorfile1:delay1...");
                 return -1;
        }
    }
	/* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    	perror_exit("socket");
	/* Find server address */
    if ((rem = gethostbyname(MirrorServerAddress)) == NULL) {	
	   herror("gethostbyname"); exit(1);
    }
    //port = atoi(argv[2]); /*Convert port number to integer*/
    server.sin_family = AF_INET;       /* Internet domain */
    memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(MirrorServerPort);         /* Server port */
    /* Initiate connection */
    if (connect(sock, serverptr, sizeof(server)) < 0)
	   perror_exit("connect");
    printf("Connecting to %s port %d\n", MirrorServerAddress, MirrorServerPort);
    if(write(sock,requests,strlen(requests))<0)
	perror_exit("write");
    bytes_read=read(sock,buf,255);
    for (i=0;i<bytes_read;i++)
	printf("%c",buf[i]); 
    close(sock);                 /* Close socket and exit */
    free(requests);
}			     

void perror_exit(char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}
