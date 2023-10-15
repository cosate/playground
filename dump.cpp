#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <linux/bpf.h>
#include <linux/filter.h>

using namespace std;

#define BUFFER_MAX_LEN 4096
#define EPOLL_MAX_NUM 1024

char buffer[BUFFER_MAX_LEN];

int main() {
    int listen_fd = ::socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    int buf_size = 65535;

    ::setsockopt(listen_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(int));
    ::setsockopt(listen_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(int));

    sock_filter code[] = {
        {0x06, 0, 0, 0x00000000},
    };

    sock_fprog bpf = {
        .len = sizeof(code) / sizeof(code[0]),
        .filter = code,
    };

    int epfd = epoll_create(1024);

    struct epoll_event event, *wait_events;
    event.events = EPOLLIN;
    event.data.fd = listen_fd;

    wait_events = (epoll_event *) malloc(sizeof(struct epoll_event) * EPOLL_MAX_NUM);

    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &event);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd;

    for(;;) {
        int active_fds_cnt = epoll_wait(epfd, wait_events, EPOLL_MAX_NUM, -1);
        for (int i =0; i < active_fds_cnt; i++) {
            if (wait_events[i].data.fd == listen_fd) {
                int n = recvfrom(listen_fd, &buffer[0], BUFFER_MAX_LEN, 0, NULL, NULL);
                for (int i = 0; i < n; i +=16) {
                    printf("%02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x %02x%02x\n",
                            buffer[i], buffer[i + 1],
                            buffer[i + 2], buffer[i + 3],
                            buffer[i + 4], buffer[i + 5],
                            buffer[i + 6], buffer[i + 7],
                            buffer[i + 8], buffer[i + 9],
                            buffer[i + 10], buffer[i + 11],
                            buffer[i + 12], buffer[i + 13],
                            buffer[i + 14], buffer[i + 15]);
                    

                    if (i + 16 >= n) {
                        break;
                    }
                }

                printf("\n\n");

                // accept
                // client_fd = ::accept(listen_fd, (struct sockaddr*) &client_addr, &client_len);

                // char ip[20];
                // printf("new connection[%s:%d]\n", inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip)), ntohs(client_addr.sin_port));

                // event.events = EPOLLIN | EPOLLOUT;
                // event.data.fd = client_fd;
                // epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &event);

            } else if (wait_events[i].events & EPOLLIN) {
                printf("epoll in\n");

                client_fd = wait_events[i].data.fd;
                buffer[0] = '\0';
                int n = ::read(client_fd, buffer, BUFFER_MAX_LEN);


            } else if (wait_events[i].events & EPOLLOUT) {
                printf("epoll out\n");
                client_fd = wait_events[i].data.fd;
                ::write(client_fd, buffer, strlen(buffer));

                event.events = EPOLLIN;
                event.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_MOD, client_fd, &event);

            }
        }
    }



}