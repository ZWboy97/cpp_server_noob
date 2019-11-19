#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h> // for struct sockaddr_in
#include <arpa/inet.h>  // for inet_addr()
#include <stdlib.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
    std::cout << "Welcome! This is a UDP server." << std::endl;

    int ret = -1;

    int socket_fd;
    // step1: create socket
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0)
    {
        std::cout << "create udp socket failed!" << std::endl;
        exit(-1);
    }
    // step: bind
    struct sockaddr_in addr;
    addr.sin_family = AF_INET; // IPV4
    addr.sin_port = 9876;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    ret = bind(socket_fd, (sockaddr *)&addr, sizeof(struct sockaddr));
    if (ret < 0)
    {
        std::cout << "bind addr and port failed!" << std::endl;
        exit(-1);
    }
    // step3: recvfrom
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int n;
    char recv_buf[BUFFER_SIZE] = {0};
    char send_buf[BUFFER_SIZE] = {0};

    while (1)
    {
        // receive data to recv_buf
        n = recvfrom(socket_fd, recv_buf, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &len);
        if (n > 0)
        {
            recv_buf[n] = 0; //转为字符串
            std::cout << "receive data:" << recv_buf << std::endl;
            n = sendto(socket_fd, send_buf, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
            if (n < 0)
            {
                std::cout << "send error!" << std::endl;
            }
        }
        else
        {
            std::cout << "receive error!" << std::endl;
        }
    }
    return 0;
}