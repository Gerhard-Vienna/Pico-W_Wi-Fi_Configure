#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

#define BUF_SIZE 256

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[BUF_SIZE];

    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("Terminate messages with \".\" and press <RTN> to send.\n");
    printf("Type: q<RTN> to exit.\n\n");
    for(;;){
        bzero(buffer,BUF_SIZE);
        printf("Please enter the message: ");

        do{
            fgets(buffer ,BUF_SIZE - strlen(buffer),stdin);
            if(buffer[0] == 'q'){
                close(sockfd);
                return 0;
            }
            if(buffer[0] == 'Q')
                abort();

            buffer[strlen(buffer) - 1] = '\0'; // delete trailing \n

            printf("Now sending \"%s\"\n", buffer);
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                perror("ERROR writing to socket");

        }while(buffer[strlen(buffer) - 1] != '.' && buffer[strlen(buffer) - 1] != '!');

        printf("Request complete, wait for response\n");
        bzero(buffer,256);
        n = read(sockfd,buffer,BUF_SIZE - 1);
        if (n < 0)
            perror("ERROR reading from socket");
        printf("Server response: \"%s\"\n",buffer);
    }
    close(sockfd);
    return 0;
}

