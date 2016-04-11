#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CLIENT_BUFFER_SIZE     1024

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: client <host> <Message>\n");
        exit(1);
    }

    int socketId = socket(PF_INET, SOCK_STREAM, 0);
    if (socketId == -1)
    {
        fprintf(stderr, "Failed: socket()\n%s\n", stderror(errno));
        exit(1);
    }

    struct sockaddr_in serverAddr;
    socklen_t addrSize = sizeof(serverAddr);
    bzero((char*)&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family       = AF_INET;
    serverAddr.sin_port         = htons(8080);
    serverAddr.sin_addr.s_addr  = inet_addr(argv[1]);
    if (connect(socketId, (struct sockaddr*)&serverAddr, addrSize) == -1)
    {
        fprintf(stderr, "Failed: connect()\n%s\n", stderror(errno));
        close(socketId);
        exit(1);
    }

    write(socketId, argv[2], strlen(argv[2]));

    shutdown(socketId, SHUT_WR);

    char    buffer[CLIENT_BUFFER_SIZE];
    size_t  get = read(socketId, buffer, CLIENT_BUFFER_SIZE - 1);

    buffer[get] = '\0';
    fprintf(stdout, "%s %s\n", "Response from server", buffer);

    close(socketId);
}
