// can use echo_client.c for client side
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>


#define BUF_SIZE 4 // Set a small buffer to show Edge Trigger
#define EPOLL_SIZE 50
#define PORT 9190

void error_handling(char * message);

void set_no_blocking_mode(int fd);

int main(int argc, char const *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t adr_sz;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0 , sizeof(serv_adr));
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

    struct epoll_event * ep_events;
    struct epoll_event event;
    int epfd, event_cnt;

    epfd = epoll_create(EPOLL_SIZE);
    ep_events = (struct epoll_event*)malloc(sizeof(struct epoll_event) * EPOLL_SIZE);

    set_no_blocking_mode(serv_sock); // set no blocking mode 
    event.events = EPOLLIN;
    event.data.fd = serv_sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);

    char buf[BUF_SIZE];
    int read_cnt;

    while (1)
    {
        event_cnt = epoll_wait(epfd, ep_events, EPOLL_SIZE, -1);

        if (event_cnt == -1)
        {
            puts("epoll_wait() error");
            break;
        }

        puts("return epoll_wait");       
        for(int i=0; i < event_cnt; i++)
        {
            if (ep_events[i].data.fd == serv_sock)
            {
                adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &adr_sz);
                set_no_blocking_mode(clnt_sock); //set no blocking mode;
                event.events = EPOLLIN | EPOLLET; // set edge trigger
                event.data.fd = clnt_sock;
                epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);
                printf("connected client: %d \n", clnt_sock);
            }
            else
            {
                while (1)
                {
                    clnt_sock = ep_events[i].data.fd;
                    read_cnt = read(clnt_sock, buf, BUF_SIZE);

                    if (read_cnt == 0) //close request
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, clnt_sock, NULL);
                        close(clnt_sock);
                        printf("closed client: %d \n", clnt_sock);
                        break;
                    }
                    else if(read_cnt < 0) // as it is no blocking mode, it will return <0 when no data is read
                    {
                        if (errno == EAGAIN)
                        {
                            break;
                        }
                    }
                    else
                    {
                        write(clnt_sock, buf, read_cnt);
                    }
                }
            }
        }
    }

    close(serv_sock);
    close(epfd);
    
    return 0;
}

void error_handling(char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void set_no_blocking_mode(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag|O_NONBLOCK);
}
