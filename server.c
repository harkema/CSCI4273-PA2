//tcpechosrv.c - A concurrent TCP echo server using threads


#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* for fgets */
#include <strings.h>     /* for bzero, bcopy */
#include <unistd.h>      /* for read, write */
#include <sys/socket.h>  /* for socket use */
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
const char delim[6] = " \n\0";

char* getFileType (char* fileName)
{
    char* fileType = strrchr(fileName,'.');

    if (fileType == NULL)
        return "na";
    if (strcmp(fileType,".html") == 0)
        fileType = "text/html";
    else if (strcmp(fileType, ".txt") == 0)
        fileType= "text/plain";
    else if (strcmp(fileType, ".png") == 0)
        fileType = "image/png";
    else if (strcmp(fileType, ".gif") == 0)
        fileType = "image/gif";
    else if (strcmp(fileType, ".jpg") == 0)
        fileType = "image/jpg";
    else if (strcmp(fileType, ".css") == 0)
        fileType = "text/css";
    else if (strcmp(fileType, ".js") == 0)
        fileType = "application/javascript";

    return fileType;
}

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1)
    {
	     connfdp = malloc(sizeof(int));
	     *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	     pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void * thread(void * vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd)
{
    char* cmd;
    char* file;
    char* fileType;
    char fileType1[32];
    char fileBuffer[MAXLINE];
    int packetSize;
    int fileSize;
    char size[32];
    size_t n;
    char buf[MAXLINE];
    char httpmsg[]="HTTP/1.1 200 Document Follows\r\nContent-Type:text/html\r\nContent-Length:32\r\n\r\n<html><h1>Hello CSCI4273 Course!</h1>";

    //Read from connfd and put in buf
    n = read(connfd, buf, MAXLINE);
    printf("server received the following request:\n%s\n",buf);
    strcpy(buf,httpmsg);
    printf("server returning a http message with the following content.\n%s\n",buf);
    write(connfd, buf,strlen(httpmsg));


    //Start processing request
    if(strlen(buf)!= 0)
    {
      cmd = strtok(buf, delim);
      file = strtok(NULL, delim);

      if(strncmp(cmd, "GET", 3) == 0 && file != NULL)
      {
        if(strncmp(file, "/" , 2))
        {
          file = "index.html";
        }
        else
        {
          file++;
        }
        printf("%s\n", file);
        fileType = getFileType(file);
        printf("%s\n", file);
        strcpy(fileType1, fileType);
        FILE *filePointer;
        filePointer = fopen(file, "rb");

        //get the size of the file
        if(filePointer > 0)
        {
          //determine the size of the file
          fseek(filePointer, 0 , SEEK_END); //set offset to the end
          fileSize = ftell(filePointer);
          rewind(filePointer); //put file position back
          sprintf(size, "%d", fileSize); //send formatted file size to size variable
        }
        else
        {
          strcpy(buf, "HTTP/1.1 500 Internal Server Error\n");
          write(connfd, buf, strlen(buf));
        }

        //construct the http message
        strcat(httpmsg, fileType1);
        strcat(httpmsg, "\r\nContent-Length:");
        strcat(httpmsg, size);
        strcat(httpmsg,"\r\n\r\n");
        printf("httpmsg:%s\n",httpmsg);

        if (filePointer != NULL)
        {
          strcpy(buf, httpmsg);
          //Indicate successful HTTP request to client
          write(connfd, buf, strlen(buf));

          while((packetSize = fread(fileBuffer,1, MAXLINE-1,filePointer)) > 0)
          {
            //Send to Client
            if(send(connfd, fileBuffer, packetSize, 0) < 0)
            {
              printf("An error has occured\n");
            }
            bzero(fileBuffer, MAXLINE);
            printf("Succesfully read %d bytes", packetSize);
          }
        }
      }

    }

}

/*
 * open_listenfd - open and return a listening socket on port
 * Returns -1 in case of failure
 */
int open_listenfd(int port)
{
    int listenfd, optval=1;
    struct sockaddr_in serveraddr;

    /* Create a socket descriptor */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    /* Eliminates "Address already in use" error from bind. */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&optval , sizeof(int)) < 0)
        return -1;

    /* listenfd will be an endpoint for all requests to port
       on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    if (bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;

    /* Make it a listening socket ready to accept connection requests */
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */
