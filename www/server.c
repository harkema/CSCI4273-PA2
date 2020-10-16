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


int open_listenfd(int port);
void echo(int connfd);
void *thread(void *vargp);
const char delim[6] = " \n\0";

//Determine what kind of file is being requested, and make sure it is associated with the proper directory
char* getTypes(char* newFile)
{
    //Get pointer to the the actual file name
    char* fileType = strrchr(newFile,'.');

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

    return fileType;
}

int main(int argc, char **argv)
{

    //Create socket structs and maintain server loop
    int listenfd, *connfdp, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
	     fprintf(stderr, "usage: %s <port>\n", argv[0]);
	     exit(0);
    }
    port = atoi(argv[1]);

    //Create socket
    listenfd = open_listenfd(port);

    while (1)
    {
	     connfdp = malloc(sizeof(int));
	     *connfdp = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
       //Use pthread to process large requests
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

    //Read from file (connfd) into buf
    n = read(connfd, buf, MAXLINE);

    printf("server received the following request:\n%s\n",buf);

    if(strlen(buf) != 0)
    {
        //Creeate tokens of the commands - should always be GET and files
        cmd = strtok(buf,delim);
        file = strtok(NULL,delim);

        if(strncmp(cmd,"GET",3) == 0 && file != NULL)
        {

          //If there is a file noted, then display index
            if(strncmp(file,"/",2) == 0)
            {
                file = "index.html"; // default webpage
            }
            else
            {
                //If not the default, move to next file
                file++;
            }

            printf("Requested File: %s\n", file);

            //Get the type of the file and associate with proper directory
            type = getTypes(file);

            //This saves the file type to be displayed within the HTTP message
            strcpy(fileType,type);
            FILE *fp; // file pointer

            //Open the file requested by the server
            fp = fopen(file,"rb");


            if(fp > 0)
            {
                //Determine the file size
                //Ptr to end of file
                fseek(fp,0,SEEK_END);
                //Returns the current file position - used to determine file size
                fSz = ftell(fp);
                //Return to original position
                rewind(fp);
                //Size == file size
                sprintf(size,"%d",fSz);
            }

            else
            {
                strcpy(buf,"HTTP/1.1 500 Internal Server Error\n");
                write(connfd, buf,strlen(buf));
            }


            //Construct the HTTP Message
            strcat(httpmsg,fileType);

            strcat(httpmsg,"\r\nContent-Length:");
            strcat(httpmsg,size);
            strcat(httpmsg,"\r\n\r\n");
            printf("httpmsg:%s\n",httpmsg);



            if(fp != NULL)
            {
                //Copy HTTP msg to buf that will be read by client
                strcpy(buf,httpmsg);

                //Write to the client
                write(connfd, buf,strlen(buf));

                //While loop to keep writing file to be viewed by client
                while((packetSize = fread(filebuffer,1,MAXLINE-1,fp)) > 0)
                {
                    if(send(connfd, filebuffer, packetSize, 0) < 0)
                    {
                        printf("Error\n");
                    }
                    bzero(filebuffer,MAXLINE);
                    printf("read %d bytes of data", packetSize);
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
    //Put in listening mode
    if (listen(listenfd, LISTENQ) < 0)
        return -1;
    return listenfd;
} /* end open_listenfd */
