#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/wait.h>        /* sockets */
#include <sys/types.h>       /* sockets */
#include <sys/socket.h>      /* sockets */
#include <netinet/in.h>      /* internet sockets */
#include <netdb.h>               /* gethostbyaddr */
#include <sys/stat.h>
struct requests
{
    char id[80];
    int delay;
};
int main(int argc,char * argv[])

{
    int port,size,option,sock,newsock,err;
    int i=0;
    struct stat s;
    char * dirorfilename;
    struct requests req[20];
    int NoOfReqs,x,bytes_read,bytes_written;
    char filename[200];
    char list_command[200];
    struct sockaddr_in server, client;
    socklen_t clientlen;
    void  *p;
    char buffer[1024];
    char command[256];
    int current_delay;
    char * search_for_delay,*requested_file;
    char *token;
    FILE * fp;
    FILE * list;
    static struct option long_options[] = {
        {"p",required_argument,NULL  ,  'q' },
        {"d",required_argument,NULL,  'x' },
        {NULL,0,NULL, 0}
    };
    struct sockaddr *serverptr=(struct sockaddr *)&server;
    struct sockaddr *clientptr=(struct sockaddr *)&client;
    struct hostent *rem;
    while ((option = getopt_long_only(argc, argv,"q:x:",long_options,NULL)) != -1)
    {
        switch (option) 
        {
             case 'q' : port=atoi(optarg);
                 break;
             case 'x' : size=strlen(optarg)+1;
                        dirorfilename=malloc(sizeof(char)*size);
                        strcpy(dirorfilename,optarg);
                 break;
            
             default: 
                 printf("must be ./ContentServer -p <port> -d <dirorfilename>");
                 return -1;
        }
    }
    NoOfReqs=1;
    clientlen=sizeof(client);
    /* Create socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("socket");
    server.sin_family = AF_INET;       /* Internet domain */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);      /* The given port */
    /* Bind socket to address */
    if (bind(sock, serverptr, sizeof(server)) < 0)
        perror("bind");
    /* Listen for connections */
    if (listen(sock, 5) < 0) perror("listen");
        printf("Listening for connections to port %d\n", port);
    while (1) 
    {
	printf("waiting to accept someone\n");
        /* accept connection */
        if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror("access");
        /* Find client's address */
        if ((rem = gethostbyaddr((char *) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
            herror("gethostbyaddr"); exit(1);}
        printf("Accepted connection from %s\n", rem->h_name);
        printf("Accepted connection\n");
        bzero(command,256);
        if(read(newsock,command,255)<0)
                perror("read");
        //printf("%s\n",command);
	token=strtok(command,"' ','\n'");
	if (!strcmp(token,"LIST"))
	{
		token=strtok(NULL,"' ','\n'");
		strcpy(req[NoOfReqs-1].id,token);
		token=strtok(NULL,"' ','\n'");
		req[NoOfReqs-1].delay=atoi(token);
		printf("delay %d and id %s\n",req[NoOfReqs-1].delay,req[NoOfReqs-1].id);
		NoOfReqs++;
		sprintf(filename,"list%s.txt",req[NoOfReqs-2].id);
                printf("in array %s\n",req[NoOfReqs-2].id);
		while(filename[i]!='\0')
		{
			if(filename[i]=='/'|| filename[i]==':')
				filename[i]='-';
			i++;
		}
        	list=fopen(filename,"ab+"); //return the requested files in a file
        	sprintf(list_command,"find %s -print > %s",dirorfilename,filename);
        	system(list_command);
        	if(write(newsock,filename,strlen(filename))<0)
                	perror("write"); 
		fclose(list);
	}
	else if(!strcmp(token,"FETCH"))
	{
		requested_file=strtok(NULL," ");
		search_for_delay=strtok(NULL," ");
		for (x=0;x<NoOfReqs;x++) //find delay
			if(!strcmp(search_for_delay,req[x].id))
				current_delay=req[x].delay;
		printf("delay for %s is %d\n",requested_file,current_delay);
		if( stat(requested_file,&s) == 0 )
		{
			printf("IT'S A FILE\n");
    			if( s.st_mode & S_IFDIR )
    			{
				if(write(newsock,"it's directory",strlen("it's directory"))<0)
                        		perror("write");
    			}
   	 		else if( s.st_mode & S_IFREG )
    			{
				if((fp=fopen(requested_file,"r"))==NULL)
					perror("fopen");
				sleep(current_delay);
        			while (1) { //write content of requested file to socket
    					bzero(buffer,1024);
					bytes_read = fread(buffer,1,1024,fp);
    					if (bytes_read == 0) // We're done reading from the file
					{
						 write(newsock,"!",1);
       						 break;
					}
					if(bytes_read==-1)
						perror("read");
    					p = buffer;
    					while (bytes_read > 0) {
        					bytes_written = write(newsock, p, bytes_read);
        					bytes_read -= bytes_written;
        					p += bytes_written;
    					}
				}
				printf("sent %s\n",requested_file);
    			}
		}
		else
			perror("stat");
	}
    }
}
