#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 11111

int
send_with_tcp(const char* reply)
{
    int                ret; 
    int                sockfd;
    int                connd;
    struct sockaddr_in servAddr;
    struct sockaddr_in clientAddr;
    socklen_t          size = sizeof(clientAddr);
    char               buff[4096];
    size_t             buff_size;
    size_t             buff_count;
    size_t             len;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "ERROR: failed to create the socket\n");
        ret = -1;
        goto end;
    }

    memset(&servAddr, 0, sizeof(servAddr));

    servAddr.sin_family      = AF_INET;
    servAddr.sin_port        = htons(PORT);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) == -1) {
        fprintf(stderr, "ERROR: failed to bind\n");
        ret = -1;
        goto servsocket_cleanup;
    }

    if (listen(sockfd, 5) == -1) {
        fprintf(stderr, "ERROR: failed to listen\n");
        ret = -1; 
        goto servsocket_cleanup;
    }

    printf("Waiting for a connection...\n");

    if ((connd = accept(sockfd, (struct sockaddr*)&clientAddr, &size))
        == -1) {
        fprintf(stderr, "ERROR: failed to accept the connection\n\n");
        ret = -1;
        goto servsocket_cleanup;
    }

    printf("Client connected successfully\n");

    while (1) {
        // size of batch
        buff_size = 0;
        if (read(connd, &buff_size, sizeof(int)) == -1) {
            fprintf(stderr, "ERROR: failed to read | size of batch\n");
            ret = -1;
            close(sockfd);
        }

        if (buff_size == *(int *)"shut") {
            printf("Shutdown command issued!\n");
            break;
        }
        printf("Client: size of batch: %ld\n", buff_size);

        // batch
        memset(buff, 0, sizeof(buff));
        if (read(connd, buff, buff_size) == -1) {
            fprintf(stderr, "ERROR: failed to read | batch\n");
            ret = -1;
            close(sockfd);
        }
        printf("Client: batch: %s\n", buff);
        
        // how many packets
        buff_count = 0;
        if (read(connd, &buff_count, sizeof(size_t)) == -1) {
            fprintf(stderr, "ERROR: failed to read | packets count\n");
            ret = -1;
            close(sockfd);
            exit(1);
        }
        printf("Client: packets count: %ld\n", buff_count);
        
        // packets
        for(int i=0; i<buff_count; i++){
            // size of packet
            buff_size = 0;
            if (read(connd, &buff_size, sizeof(int)) == -1) {
                fprintf(stderr, "ERROR: failed to read | size of packet\n");
                ret = -1;
                close(sockfd);
            }
            printf("Client: size of packet[%d]: %ld\n", i, buff_size);

            // packet
            memset(buff, 0, sizeof(buff));
            if (read(connd, buff, buff_size) == -1) {
                fprintf(stderr, "ERROR: failed to read | packet\n");
                ret = -1;
                close(sockfd);
            }
            printf("Client: packet[%d]: %s\n", i, buff);
        }

//        memset(buff, 0, sizeof(buff));
//        memcpy(buff, reply, strlen(reply));
//        len = strnlen(buff, sizeof(buff));
//
//        if ((ret = write(connd, buff, len)) != len) {
//            fprintf(stderr, "ERROR: failed to write\n");
//            goto clientsocket_cleanup;
//        }

    }

    close(connd);

    printf("Shutdown complete\n");
clientsocket_cleanup:
    close(connd);
servsocket_cleanup:
    close(sockfd);
end:
    return ret;

    close(sockfd);
}

int main(void){
    const char* reply = "I hear ya fa shizzle!";
    send_with_tcp(reply);
    return 0;
}