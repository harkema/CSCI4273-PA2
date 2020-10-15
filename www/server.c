/*
 * tcpechosrv.c - A concurrent TCP echo server using threads
 */

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
#include <sys/stat.h>

#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

/*
.html = text/html
.txt = text/plain
.png = image/png
.gif = image/gif
.jpg = image/jpg
.css = text/css
.js = application/javascript
*/


int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
const char delim[6] = " \n\0";

char* getTypes(char* newFile)
{
    char* fileType = strrchr(newFile,'.');
    //printf("%s\n\n\n",fileType);
    if (fileType == NULL)
        return "na";
    if (strcmp(fileType,".html") == 0)
        fileType = "text/html";
    else if (strcmp(fileType, ".txt") == 0)
        fileType = "text/plain";
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
    //printf("%s\n\n\n",fileType);
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
    while (1) {
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
    size_t n;
    char* cmd;
    char* file;
    char* type;
    char fileType[32];
    char buf[MAXLINE];
    char filebuffer[MAXLINE];
    int packetSize;
    int fSz;
    char size[32];
    char httpmsg[MAXLINE] = "HTTP/1.1 200 Document Follows\r\nContent-Type:";

    n = read(connfd, buf, MAXLINE); // connection and request coming into the server from the client into the buffer
    printf("server received the following request:\n%s\n",buf);

    printf("strlen of buffer %d\n", strlen(buf));
    if(strlen(buf) != 0)
    {
        cmd = strtok(buf,delim); // tokenize the command (in this case always a GET)
        file = strtok(NULL,delim); // tokenize the requested file

        if(strncmp(cmd,"GET",3) == 0 && file != NULL)
        {

            if(strncmp(file,"/",2) == 0)
            {
                file = "index.html"; // default webpage
            }
            else
            {
                file++; // moves the char array pointer over 1 to get rid of initial / (slash)
            }

            printf("FILE: %s\n", file);
            type = getTypes(file); // function that will determine the type of file being requested
            printf("FILE: %s\n", file);
            strcpy(fileType,type);
            FILE *fp; // file pointer
            fp = fopen(file,"rb"); // open the file


            if(fp > 0)
            {
                /* quick little trick to determine the size of the file */
                fseek(fp,0,SEEK_END);
                fSz = ftell(fp);
                rewind(fp);
                sprintf(size,"%d",fSz); // set our size variable to the file size
            }

            else
            {
                strcpy(buf,"HTTP/1.1 500 Internal Server Error\n");
                write(connfd, buf,strlen(buf));
            }


            /* put all of the details of the httpmsg together */
            strcat(httpmsg,fileType);
            strcat(httpmsg,"\r\nContent-Length:");
            strcat(httpmsg,size);
            strcat(httpmsg,"\r\n\r\n");
            printf("httpmsg:%s\n",httpmsg);



            if(fp != NULL)
            {
                strcpy(buf,httpmsg);
                write(connfd, buf,strlen(buf)); // send the httpmsg off to the client
                while((packetSize = fread(filebuffer,1,MAXLINE-1,fp)) > 0) // while there is still file data to be read
                {
                    if(send(connfd, filebuffer, packetSize, 0) < 0) // send the file data off to the client
                    {
                        printf("Error\n");
                    }
                    bzero(filebuffer,MAXLINE); // zero out the buffer
                    printf("read %d bytes of data", packetSize); // housekeeping
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
