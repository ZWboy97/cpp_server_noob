#include <iostream>
#include <sys/socket.h>  // for socket(),setsockopt
#include <netinet/in.h>  // for struct sockaddr_in
#include <stdlib.h>      // for exit()
#include <unistd.h>      // for close()
#include <string.h>      // for bzero()
#include <fcntl.h>       // for fcntl()
#include <sys/epoll.h>   // for epoll
#include <errno.h>       // for errno
#define PORT 8881        // tcp 端口
#define MESSAGE_LEN 1024 // 消息大小
#define FD_SIZE 20       // epoll event 大小
#define MAX_EVENTS 20
#define TIMEOUT 500
int main(int argc, char *argv[])
{
    // 结果
    int ret = -1;
    // 连接和发送的fd
    int socket_fd = -1;
    // 用于接收的fd
    int accept_fd = -1;
    // epoll的fd
    int epoll_fd;
    //缓冲长度，与并发量相关
    int backlog = 10;
    // socket连接的本地地址以及client端的地址
    struct sockaddr_in localaddr, remoteaddr;
    // setsockopt的参数
    int on = 1;
    // 向epoll中添加fd以及其event
    struct epoll_event event;
    // epoll返回的事件数目
    int event_number;
    //epoll返回的事件列表
    struct epoll_event events[FD_SIZE];

    //step1: 创建socket
    socket_fd = socket(PF_INET, SOCK_STREAM, 0); // ipv4，TCP连接
    if (socket_fd == -1)
    {
        std::cout << "failed to create socket!" << std::endl;
        exit(-1);
    }
    // 套接字参数，REUSEADDR地址复用
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                     &on, sizeof(on));
    if (ret == -1)
    {
        std::cout << "failed to set socket options!" << std::endl;
    }
    // 将socket设置为非阻塞，select与epoll要求
    int flags = fcntl(socket_fd, F_GETFL, 0);      // 获取flags
    fcntl(socket_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
    // step2： bind（）
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = PORT;
    localaddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(localaddr.sin_zero), 8);
    ret = bind(socket_fd, (struct sockaddr *)&localaddr, sizeof(struct sockaddr));
    if (ret == -1)
    {
        std::cout << "failed to bind " << std::endl;
        exit(-1);
    }
    // step3： listen()
    ret = listen(socket_fd, backlog);
    if (ret == -1)
    {
        std::cout << "failed to listen " << std::endl;
        exit(-1);
    }

    // 创建epoll
    epoll_fd = epoll_create(256);
    // 将socket_fd的事件添加到epoll中
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event); // 将侦听的事件添加

    // 进入server 服务循环
    while (1)
    {
        // 阻塞，返回事件数目，返回的events列表，MAX_EVENTS，最大返回的事件数目，TIMEOUT,超时时间
        event_number = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT);
        for (int i = 0; i < event_number; i++)
        {
            if (events[i].data.fd == socket_fd) // 是新的连接
            {
                // 创建accept_fd,添加到accept_fds中，之后交给select监听
                std::cout << "new connection, create accept_fd!" << std::endl;
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr *)&remoteaddr, &addr_len);
                // Epoll也需要设置为非阻塞
                int flags = fcntl(accept_fd, F_GETFL, 0);      // 获取flags
                fcntl(accept_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
                // 将其添加到epoll中去
                event.events = EPOLLIN || EPOLLET; // 入，边缘触发(数据不必必须读完)
                event.data.fd = accept_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &event);
            }
            else if (events[i].events & EPOLLIN) // 输入事件
            {
                char in_buff[MESSAGE_LEN] = {0};
                memset(in_buff, 0, MESSAGE_LEN); // 清空inbuff
                ret = recv(events[i].data.fd, (void *)in_buff, MESSAGE_LEN, 0);
                if (ret == MESSAGE_LEN) // 缓冲区满了,因为设置了边缘触发
                {
                    std::cout << "maybe have data...";
                }
                // 接收缓冲区
                if (ret <= 0)
                {
                    switch (errno)
                    {
                    case EAGAIN: //说明暂时已经没有数据了，要等通知,继续循环
                        break;
                    case EINTR: //被终断了，再接收一次
                        std::cout << "recv EINTR... " << std::endl;
                        ret = recv(events[i].data.fd, &in_buff, MESSAGE_LEN, 0);
                        break;
                    default:
                        std::cout << "the client is closed" << std::endl;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                        close(events[i].data.fd);
                        break;
                    }
                }
                std::cout << "receive:" << in_buff << std::endl;
                // step6： send back
                send(events[i].data.fd, (void *)in_buff, MESSAGE_LEN, 0);
            }
        }
    }
    std::cout << "quit server" << std::endl;
    close(socket_fd);
    return 0;
}