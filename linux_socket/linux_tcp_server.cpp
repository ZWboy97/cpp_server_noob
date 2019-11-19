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
    struct sockaddr_in localaddr, remoteaddr;

    char in_buff[MESSAGE_LEN] = {
        0,
    }; // 接收缓冲区

    socket_fd = socket(PF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
    {
        std::cout << "failed to create socket!" << std::endl;
        exit(-1);
    }
    /* Set socket FD's option OPTNAME at protocol level LEVEL
   to *OPTVAL (which is OPTLEN bytes long).
   Returns 0 on success, -1 for errors.  */
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                     &on, sizeof(on));
    if (ret == -1)
    {
        std::cout << "failed to set socket options!" << std::endl;
    }
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
    ret = listen(socket_fd, backlog);
    if (ret == -1)
    {
        std::cout << "failed to listen " << std::endl;
        exit(-1);
    }
    for (;;)
    {
        socklen_t addr_len = sizeof(struct sockaddr);
        accept_fd = accept(socket_fd,
                           (struct sockaddr *)&remoteaddr,
                           &addr_len);
        int pid = fork();
        for (;;)
        {
            ret = recv(accept_fd, (void *)in_buff, MESSAGE_LEN, 0);
            if (ret == 0) // 说明没数据了
            {
                std::cout << "recv finish，end！ " << std::endl;
                break;
            }
            std::cout << "receive:" << in_buff << std::endl;
            // 返回客户端
            send(accept_fd, (void *)in_buff, MESSAGE_LEN, 0);
        }
        close(accept_fd);
    }
    close(socket_fd);
    return 0;
}