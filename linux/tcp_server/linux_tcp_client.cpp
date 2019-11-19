#include <iostream>
#include <sys/socket.h>  //for socket()
#include <sys/types.h>   // for connect()
#include <netinet/in.h>  // for struct sockaddr_in
#include <arpa/inet.h>   // for inet_addr()
#include <stdio.h>       // for gets()
#include <string.h>      // for strlen()
#include <stdlib.h>      // for exit()
#define PORT 8881        // tcp server port
#define MESSAGE_LEN 1024 // buffer size

int main(int argc, char *argv[])
{
    int socket_fd; // file descriptor
    int ret = -1;
    // step1:create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0)
    {
        std::cout << "failed to create socket" << std::endl;
        exit(-1);
    }
    // step2: connect
    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = PORT;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr));
    if (ret < 0)
    {
        std::cout << "failed to connect server" << std::endl;
        exit(-1);
    }
    // step3: send()
    char sendbuf[MESSAGE_LEN] = {
        0,
    };
    char recvbuf[MESSAGE_LEN] = {
        0,
    };
    while (1)
    {
        memset(sendbuf, 0, MESSAGE_LEN); // 清空buf
        gets(sendbuf);                   // input from console
        ret = send(socket_fd, sendbuf, strlen(sendbuf), 0);
        if (ret <= 0) // 无发送数据
        {
            std::cout << "failed to send data!" << std::endl;
            break;
        }
        if (strcmp(sendbuf, "q") == 0) // input q, quit
        {
            break;
        }
        ret = recv(socket_fd, recvbuf, MESSAGE_LEN, 0);
        recvbuf[ret] = '\0'; //末尾加\0转化为字符串
        std::cout << "receive:" << recvbuf << std::endl;
    }
    return 0;
}