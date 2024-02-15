#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>


#define BUF_SIZE 1024
#define PORT 9190

void error_handling(char* message);

int main(int argc, char const *argv[])
{
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];

    struct sockaddr_in serv_adr, clnt_adr;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        error_handling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(PORT);
    
    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)
    {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1)
    {
        error_handling("listen() error");
    }

    socklen_t clnt_adr_sz = sizeof(clnt_adr);

    printf("server started. waiting for connection...\n");

    for (int i = 0; i < 5; i++)
    {
        clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
        if (clnt_sock == -1)
        {
            error_handling("accept() error");
        }
        else
        {
            printf("Connected client %d \n", i+1);
        }

        int strlen = 0;
        while((strlen = read(clnt_sock, message, BUF_SIZE)) != 0)
        {
            write(clnt_sock, message, strlen);
        }

        close(clnt_sock);
    }

    close(serv_sock);
    return 0;
}

void error_handling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
