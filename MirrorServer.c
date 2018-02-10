/*inet_str_server.c: Internet stream sockets server */ 
#include <stdio.h>
#include <sys/wait.h> /* sockets */ 
#include <sys/types.h> /* sockets */ 
#include <errno.h>
#include <sys/socket.h> /* sockets */ 
#include <netinet/in.h> /* internet sockets */ 
#include <netdb.h> /* gethostbyaddr */ 
#include <unistd.h> /* fork */ 
#include  <stdlib.h> /* exit */ 
#include <ctype.h> /* toupper */ 
#include <signal.h> /* signal */ 
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <dirent.h>
#define BUFFER_SIZE 5
#define MAX_PRODUCERS 5
#define MAX_CONSUMERS 5
void perror_exit(char * message);
struct buffer_element {
  char string[200];
  int state; //0 for empty 1 for full
};
int counter = 0; //posa kelia tou buffer einai gemata
int in=0;
int endOfWorkers=0;
int NumDevicesDone=0;
int out=0;
int NotWorking=0;
int bytesTransferred=0;
int filesTransferred=0;
int MirrorCounter=0;
double Average=0;
double Variance=0;
double Past_Average;
struct buffer_element sharedbuffer[BUFFER_SIZE];
static pthread_cond_t cons_var;
static pthread_cond_t prod_var;
static pthread_cond_t AllDone;
static pthread_mutex_t mtx;
static pthread_mutex_t statistics;
struct files_per_request {
  char id[200];
  int files;
  int endOfFile;
};
struct files_per_request fpr[40];
struct arg_struct {
  int cell;
  char token[200];
};

void fetch(char * store_file,char * directory)
{
    char * file, * temp, * address, * port;
    int i, sock,bytes_read,create_path,is_file;
    struct sockaddr_in server;
    struct sockaddr * serverptr = (struct sockaddr * ) & server;
    FILE * fp;
    struct hostent * rem;
    char id[100];
    char buffer[1024];
    char destination_file[100];
    char make_dirs[100];
    char new_dir[100];
    DIR * dir;
    int end,total_bytes;
    char command2[200];
    char * rest=store_file;
    printf("in fetch\n");
    printf("%s\n",store_file);
    file = strtok_r(rest, " ",&rest);
    temp = strtok_r(rest, " ",&rest);
    strcpy(id, temp);
    rest=temp;
    address = strtok_r(rest, ":",&rest);
    port = strtok_r(rest, ":",&rest);
    sprintf(command2, "FETCH %s %s", file, id);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      perror_exit("socket");
    if ((rem = gethostbyname(address)) == NULL) {
      herror("gethostbyname");
      exit(1);
    }
    server.sin_family = AF_INET;
    memcpy( &server.sin_addr, rem->h_addr, rem->h_length);
    server.sin_port = htons(atoi(port));
    if (connect(sock, serverptr, sizeof(server)) < 0)
      perror_exit("connect");
    printf("Connecting from worker to %s port %d\n", address, atoi(port));
    if (write(sock, command2, strlen(command2)) < 0)
      perror_exit("write");
    create_path=0;
    bzero(buffer, 1024);
    end=0;
    is_file=0;
    total_bytes=0;
    if(strcmp(directory,file))
    {
    	sprintf(destination_file, "%s/%s", (char * ) directory, file);  //place folder or file under the requested path
        bytes_read=read(sock,buffer,1024);
    	while (bytes_read>0) 
    	{
		total_bytes+=bytes_read;
    		if (strcmp(buffer, "it's directory")) //make directories of this path in case they don't exist
        	{
			if(!create_path)
			{
				i=0;
				bzero(new_dir,100);
				while(destination_file[i]!='\0')
				{
					if(destination_file[i]=='/')
					{
						dir=opendir(new_dir);
						if(dir)
							closedir(dir); //dir exists
						else if( ENOENT==errno)  //dir doesn't exist
						{
							new_dir[i+1]='\0';
							sprintf(make_dirs,"mkdir %s",new_dir);
							system(make_dirs);
						}
					}
					new_dir[i]=destination_file[i];
					i++;
				}
			}
                        printf("DEST FILE%s\n",destination_file);
        		fp = fopen(destination_file, "a"); 
			if (buffer[bytes_read-1]=='!') //detect end of this file
			{	
				is_file=1;
				buffer[bytes_read-1]='\0';
				end=1; //end of file
			} 
        		fwrite(buffer, 1, bytes_read, fp);
        		fflush(fp);
			fclose(fp);
      		} 
        	else  //if it's directory
		{
			is_file=0;
			bzero(command2,200);
	  		sprintf(command2,"mkdir %s",destination_file);
	  		system(command2);
			total_bytes=0;
	  		end=1;
      		}
	printf("consumer %ld read_bytes %d str=%s\n",pthread_self(),bytes_read,destination_file); 
	if(end==1)
		break;
	bytes_read=read(sock,buffer,1024);
   	}
   }
   else
	bzero(destination_file,100);
   pthread_mutex_lock(&statistics);
   if(is_file==1)
   {
   	filesTransferred++;
   	bytesTransferred+=total_bytes;
        printf("bytesTransferred %d\n",bytesTransferred);
	if(Average==0)
	{	Average=(float)bytesTransferred/(float)filesTransferred;
	}
	else
	{
		Past_Average=Average;
   		Average=(1.0/(float)filesTransferred)*(bytesTransferred-Past_Average)+Past_Average;
	}
   	Variance=Variance+(bytesTransferred-Past_Average)*(bytesTransferred-Average);
   }
   for(i=0;i<40;i++)
   {       if(!strcmp(fpr[i].id,id))  //decrease the remaining files of each request
           {
                fpr[i].files--;
                break;
            }
    }
   i=0;
   while(fpr[i].endOfFile!=2)
   {
            if(fpr[i].files==0&&fpr[i].endOfFile==1)
                NumDevicesDone++;
   	    i++;
   }
   printf("devices %d\n",NumDevicesDone);
   if(NumDevicesDone==MirrorCounter-1)
   {
        pthread_cond_signal(&AllDone);
        endOfWorkers=1;
	pthread_mutex_unlock(&statistics);
	printf("end of a worker %ld\n",pthread_self());
	return;
   }
   else
	NumDevicesDone=NotWorking; //make counter ready for next loop
   pthread_mutex_unlock(&statistics);

   printf("i returned from %ld with file %s\n",pthread_self(),destination_file);
   return;
}

void * workers(void * directory) {
  char store_file[200];
  printf("consumers wait to take lock\n");
  while (1) {
     pthread_mutex_lock(&mtx);
     while(counter<=0&&endOfWorkers==0)
	pthread_cond_wait(&cons_var,&mtx);
     if(endOfWorkers==1)
     {	
	pthread_cond_signal(&cons_var);
	pthread_mutex_unlock(&mtx);
	pthread_exit(NULL);
     }
     strcpy(store_file,sharedbuffer[out].string);
     out++;
     out%=BUFFER_SIZE;
     counter--;
     pthread_cond_signal(&prod_var);
     pthread_mutex_unlock(&mtx);
     fetch(store_file,directory);
  }
}

void produce(char * requested_file) {
  while(1)
  {
	printf("producers wait to take lock\n");
        getchar();
	if(!pthread_mutex_trylock(&mtx)){
	while(counter>=BUFFER_SIZE)
		pthread_cond_wait(&prod_var,&mtx);
	strcpy(sharedbuffer[in].string,requested_file);
	in++;
	in%=BUFFER_SIZE;
	counter++;
	pthread_cond_signal(&cons_var);
	pthread_mutex_unlock(&mtx);
	return;}
  }
}
void * MirrorManagers(void * arguements) {
  char * address, * port, * directory, * delay;
  struct arg_struct * args = arguements;
  printf("In manager %s with cell %d\n", args->token, args->cell);
  char parse[] = ":,'\0'";
  int sock;
  FILE * fp;
  char * line = NULL;
  char requested[400];
  size_t len = 0;
  char list[256];
  struct sockaddr_in server;
  struct sockaddr * serverptr = (struct sockaddr * ) &server;
  struct hostent * rem;
  char command1[200];
  ssize_t chars;
  char ContentServerID[100];
  char *rest = args->token;
  strcpy(ContentServerID, args->token);
  address = strtok_r(rest,parse,&rest);
  port = strtok_r(rest, parse,&rest);
  directory = strtok_r(rest, parse,&rest);
  delay = strtok_r(rest, parse,&rest);
  sprintf(command1, "LIST %s %d", ContentServerID, atoi(delay));
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror_exit("socket");
  /* Find server address */
  if ((rem = gethostbyname(address)) == NULL) {
    herror("gethostbyname");
    exit(1);
  }
  server.sin_family = AF_INET; /* Internet domain */
  memcpy( &server.sin_addr, rem->h_addr, rem->h_length);
  server.sin_port = htons(atoi(port)); /* Server port */
  if (connect(sock, serverptr, sizeof(server)) < 0)
  {
	pthread_mutex_lock(&statistics);
	NotWorking++;
	pthread_mutex_unlock(&statistics);
	pthread_exit(address);
  }
  printf("Connecting to %s port %d\n", address, atoi(port));
  if (write(sock, command1, strlen(command1)) < 0)
    perror_exit("write");
  bzero(list, 256);
  if (read(sock, list, 255) < 0)
    perror("read");
  fp = fopen(list, "r");
  fpr[args->cell].endOfFile = 0;
  strcpy(fpr[args->cell].id,ContentServerID);
  while (1) 
  {
    	chars = getline(&line,&len, fp);
	if (chars == -1)
        {
                fpr[args->cell].endOfFile=1;
                break;
        }

    	if (strncmp(line, directory, strlen(directory)) == 0) 
    	{
        	if (line[chars - 1] == '\n') //remove newline
        		line[chars - 1] = '\0';
        	sprintf(requested, "%s %s", line, ContentServerID);
        	printf("requested %s\n", requested);
        	fpr[args->cell].files++;
		produce(requested);
      	}
  }
  free(line);
  fclose(fp);
  pthread_exit(NULL);
}
int main(int argc, char * argv[]) {
  int sock, newsock, i, err;
  struct sockaddr_in server, client;
  socklen_t clientlen;
  char make_directory[100];
  pthread_t * workers_id, * MirrorManagers_id;
  char * token;
  int wrong_servers;
  void * status;
  struct arg_struct * args;
  struct sockaddr * serverptr = (struct sockaddr * ) &server;
  struct sockaddr * clientptr = (struct sockaddr * ) &client;
  struct hostent * rem;
  int size, option, threadnum, s_port;
  char * dirname;
  char requests[256];
  char results[400];
  static struct option long_options[] = {
    {"p",required_argument,NULL, 'q'},
    {"m",required_argument,NULL, 'x'},
    {"w",required_argument,NULL, 'y'},
    {NULL,0,NULL,0}
  };

  while ((option = getopt_long_only(argc, argv, "q:x:y:", long_options, NULL)) != -1) {
    switch (option) {
    case 'q':
      s_port = atoi(optarg);
      break;
    case 'x':
      size = strlen(optarg) + 1;
      dirname = malloc(sizeof(char) * size);
      strcpy(dirname, optarg);
      break;
    case 'y':
      threadnum = atoi(optarg);

      break;
    default:
      printf("must be ./MirrorServer -p <port> -m <dirname> -w <threadnum>");
      return -1;
    }
  }
  sprintf(make_directory, "mkdir %s", dirname);
  system(make_directory);
  pthread_mutex_init( & mtx, NULL);
  pthread_mutex_init(&statistics,NULL);
  pthread_cond_init(&cons_var,NULL);
  pthread_cond_init(&prod_var,NULL);
  pthread_cond_init(&AllDone,NULL);
  for (i=0;i<40;i++)
  {
	fpr[i].files=0;
	fpr[i].endOfFile=2;
  }
  for (i = 0; i < BUFFER_SIZE; i++) {
    sharedbuffer[i].state = 0;
  }
  MirrorCounter = 1;
  workers_id = malloc(threadnum * sizeof(pthread_t));
  MirrorManagers_id = malloc(MirrorCounter * sizeof(pthread_t));
  clientlen = sizeof(client);
  /* Create socket */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    perror_exit("socket");
  server.sin_family = AF_INET; /* Internet domain */
  server.sin_addr.s_addr = htonl(INADDR_ANY);
  server.sin_port = htons(s_port); /* The given port */
  /* Bind socket to address */
  if (bind(sock, serverptr, sizeof(server)) < 0)
    perror_exit("bind");
  /* Listen for connections */
  if (listen(sock, 5) < 0) perror_exit("listen");
  printf("Listening for connections to port %d\n", s_port);
  while (1) {
    /* accept connection */
    if ((newsock = accept(sock, clientptr, &clientlen)) < 0) perror_exit("accept");
    /* Find client's address */
    if ((rem = gethostbyaddr((char * ) &client.sin_addr.s_addr, sizeof(client.sin_addr.s_addr), client.sin_family)) == NULL) {
      herror("gethostbyaddr");
      exit(1);
    }
    printf("Accepted connection from %s\n", rem->h_name);
    printf("Accepted connection\n");
    bzero(requests, 256);
    if (read(newsock, requests, 255) < 0)
      perror_exit("read");
    printf("%s\n", requests);
    token = strtok(requests, ",");
    while (token != NULL) {
      printf("TOKEN %s\n", token);
      args = malloc(sizeof( * args));
      args->cell = MirrorCounter - 1;
      strcpy(args->token, token);
      if (err = pthread_create(MirrorManagers_id + MirrorCounter - 1, NULL, MirrorManagers, (void * ) args)) {
        /*  Create  a  thread  */
        printf("pthread_create error\n");
        exit(1);
      }
      MirrorCounter++;
      MirrorManagers_id = realloc(MirrorManagers_id, MirrorCounter * sizeof(pthread_t));
      token = strtok(NULL, ",");
      //printf("TOKEN %s\n",token);
    }
    for (i = 0; i < threadnum; i++) {
      if (err = pthread_create(workers_id + i, NULL, workers, (void * ) dirname)) {
        /*  Create  a  thread  */
        printf("pthread_create error\n");
        exit(1);
      }
    }
    wrong_servers=0;
    for (i = 0; i < MirrorCounter - 1; i++) {
      if (err = pthread_join( *(MirrorManagers_id + i), &status)) {
        /*  Wait  for  thread  termination  */
        printf("pthread_join error\n");
        exit(1);
      }
      if(status!=NULL)
      {
   	printf("Content Server %s doesn't exist\n",status);
	wrong_servers++;
      }

    }
    if(wrong_servers==MirrorCounter-1)
	return -1;
    pthread_cond_wait(&AllDone,&statistics);
    for (i = 0; i < threadnum; i++) {
      if (err = pthread_join( *(workers_id + i), NULL)) {
        //  Wait  for  thread  termination  
        printf("pthread_join error\n");
        exit(1);
      }
    }
    sprintf(results,"Files Transferred: %d\nBytes Transferred: %d\nNumDevicesDone: %d\nWrong Devices: %d\nAverage: %f\nVariance:%f \n",filesTransferred,bytesTransferred,NumDevicesDone,wrong_servers,Average,Variance);
    write(newsock,results,strlen(results));
    close(newsock); /* parent closes socket to client */
    break;
  }
  pthread_mutex_destroy(&mtx);
  pthread_mutex_destroy(&statistics);
  pthread_cond_destroy(&cons_var);
  pthread_cond_destroy(&prod_var);
  pthread_cond_destroy(&AllDone);
  free(dirname);
  free(MirrorManagers_id);
  free(workers_id);
}

void perror_exit(char * message) {
  perror(message);
  exit(EXIT_FAILURE);
}
