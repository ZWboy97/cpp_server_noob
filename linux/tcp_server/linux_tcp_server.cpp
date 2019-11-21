#include <iostream>
#include <sys/socket.h> // for socket(),setsockopt
#include <netinet/in.h> // for struct sockaddr_in
#include <stdlib.h>     // for exit()
#include <unistd.h>     // for close()
#include <string.h>     // for bzero()
#define PORT 8881
#define MESSAGE_LEN 1024
int main(int argc, char *argv[])
{
    int ret = -1;
    int on = 1;
    int socket_fd, accept_fd;
    int backlog = 10; //缓冲长度，与并发量相关
    pid_t pid;
    struct sockaddr_in localaddr, remoteaddr;

    char in_buff[MESSAGE_LEN] = {
        0,
    }; // 接收缓冲区
    // step1： 创建socket
    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        std::cout << "failed to create socket!" << std::endl;
        exit(-1);
    }
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, // 套接字参数，地址复用
                     &on, sizeof(on));
    if (ret == -1)
    {
        std::cout << "failed to set socket options!" << std::endl;
    }
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = PORT;
    localaddr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(localaddr.sin_zero), 8);
    // step2： bind（）
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
    for (;;)
    {
        // step4：accept
        socklen_t addr_len = sizeof(struct sockaddr);
        accept_fd = accept(socket_fd,
                           (struct sockaddr *)&remoteaddr,
                           &addr_len); // 阻塞
        // fork 一个子进程进程处理
        pid = fork();
        if (pid == 0) // pid为0，说明是子进程
        {
            for (;;)
            {
                // step5： recv
                ret = recv(accept_fd, (void *)in_buff, MESSAGE_LEN, 0);
                if (ret == 0) // 说明没数据了
                {
                    std::cout << "recv finish，end！ " << std::endl;
                    break;
                }
                std::cout << "receive:" << in_buff << std::endl;
                // step6： return send
                send(accept_fd, (void *)in_buff, MESSAGE_LEN, 0);
            }
            // step7： close()
            close(accept_fd);
        }
    }
    if (pid != 0) // 子进程中不关闭连接
    {
        close(socket_fd);
    }
    return 0;
}