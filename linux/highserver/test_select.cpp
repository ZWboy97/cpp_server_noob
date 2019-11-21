#include <iostream>
#include <sys/socket.h>  // for socket(),setsockopt
#include <netinet/in.h>  // for struct sockaddr_in
#include <stdlib.h>      // for exit()
#include <unistd.h>      // for close()
#include <string.h>      // for bzero()
#include <fcntl.h>       // for fcntl()
#define PORT 8881        // tcp 端口
#define MESSAGE_LEN 1024 // 消息大小
#define FD_SIZE 1024     // fd集大小
int main(int argc, char *argv[])
{
    // 结果
    int ret = -1;
    // 连接和发送的fd
    int socket_fd = -1;
    // 用于接收的fd
    int accept_fd = -1;
    //缓冲长度，与并发量相关
    int backlog = 10;
    // socket连接的本地地址以及client端的地址
    struct sockaddr_in localaddr, remoteaddr;
    // setsockopt的参数
    int on = 1;
    // select返回
    int events = 0;
    // 最大的fd，select的参数，为了避免将整个fd集载入
    int max_fd = -1;
    // 接收fd池中的空槽索引
    int curpos = -1;
    // accept_fds中被分配的最大index，目的是减小遍历次数，否则就要遍历FD_SIZE次
    int maxpos = 0;
    // 文件描述符集，select遍历的fd集
    fd_set fd_sets;
    // 用于接收的fd池，默认每个fd为-1，交给select来监听
    int accept_fds[FD_SIZE] = {-1}; // FD_SIZE=1024，所以最多支持1024个连接

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
    // 将socket设置为非阻塞，select要求
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
    // step3： listen（）
    ret = listen(socket_fd, backlog);
    if (ret == -1)
    {
        std::cout << "failed to listen " << std::endl;
        exit(-1);
    }
    // 初始化
    max_fd = socket_fd;
    // 将连接fd集初始化，之后交给select监听
    for (int i = 0; i < FD_SIZE; i++)
    {
        accept_fds[i] = -1;
    }
    // 进入server 服务循环
    while (1)
    {
        FD_ZERO(&fd_sets);           // 清空fd集
        FD_SET(socket_fd, &fd_sets); // 将socket_fd加入到fd集合中，之后交给select监听事件
        // 遍历accept_fdss，找到有变化的accept_fd，添加到fd_sets中
        for (int i = 0; i < maxpos; i++)
        {
            if (accept_fds[i] != -1)
            {
                if (accept_fds[i] > max_fd) // 更新max_fd
                {
                    max_fd = accept_fds[i];
                }
                FD_SET(accept_fds[i], &fd_sets);
            }
        }
        // select，阻塞监听fd_sets中的fd，处理准备就绪的io
        // fd_sets即是输入，也是函数输出，输出的fd_sets包括所有准备就绪的fd
        events = select(max_fd + 1, &fd_sets, NULL, NULL, NULL);
        if (events < 0)
        {
            std::cout << "failed to use select" << std::endl;
            break;
        }
        else if (events == 0)
        {
            std::cout << "timeout..." << std::endl;
            continue;
        }
        else if (events) // 有个fd就绪了
        {
            // 需要区别对待新的连接和数据
            if (FD_ISSET(socket_fd, &fd_sets)) // socket_fd 在准备就绪的fd_sets，则是连接来了
            {
                std::cout << "new connection!" << std::endl;
                int i;
                for (i = 0; i < FD_SIZE; i++) // 从accpet_fds找到一个空槽，分配一个给新连接
                {
                    if (accept_fds[i] == -1)
                    {
                        curpos = i;
                        std::cout << "allocate pos= !" << curpos << std::endl;
                        break;
                    }
                }
                if (i == FD_SIZE)
                {
                    std::cout << "the connection is full!" << std::endl;
                    continue;
                }
                // 创建accept_fd,添加到accept_fds中，之后交给select监听
                std::cout << "create accept_fd, and add to accept_fds!" << std::endl;
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr *)&remoteaddr, &addr_len);
                int flags = fcntl(accept_fd, F_GETFL, 0);      // 获取flags
                fcntl(accept_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
                accept_fds[curpos] = accept_fd;
                if (curpos + 1 > maxpos)
                {
                    maxpos = curpos + 1;
                }
                if (accept_fd > max_fd)
                {
                    max_fd = accept_fd;
                }
            }

            for (int i = 0; i < maxpos; i++)
            {
                if (accept_fds[i] != -1 && FD_ISSET(accept_fds[i], &fd_sets))
                {
                    char in_buff[MESSAGE_LEN] = {
                        0,
                    };                               // 接收缓冲区
                    memset(in_buff, 0, MESSAGE_LEN); // 清空inbuff
                    int ret = recv(accept_fds[i], (void *)in_buff, MESSAGE_LEN, 0);
                    if (ret == 0) // 说明没数据了
                    {
                        std::cout << "recv finish，end！ " << std::endl;
                        close(accept_fds[i]);
                        accept_fds[i] = -1;
                    }
                    std::cout << "receive:" << in_buff << std::endl;
                    // step6： return send
                    send(accept_fds[i], (void *)in_buff, MESSAGE_LEN, 0);
                }
            }
        }
    }
    std::cout << "quit server" << std::endl;
    close(socket_fd);
    return 0;
}