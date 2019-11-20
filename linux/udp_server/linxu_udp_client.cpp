#include <iostream>
#include <sys/socket.h> // for socket()
#include <stdlib.h>     // for exit()
#include <netinet/in.h> // for struct sockaddr_in
#include <arpa/inet.h>  // for inet_addr()
#include <string.h>     // for strlen
#include <unistd.h>     // for close()

#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{

    // step1: socket
    int socket_fq;
    socket_fq = socket(AF_INET, SOCK_DGRAM, 0); // UDP是基于报文的
    if (socket_fq < 0)
    {
        std::cout << "create socket failed!" << std::endl;
        exit(-1);
    }
    // step2:
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = 9876;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    char send_buf[BUFFER_SIZE] = "hello";
    char recv_buf[BUFFER_SIZE] = {0};
    socklen_t len = sizeof(serverAddr);
    int n = 0;

    if (serverAddr.sin_addr.s_addr == INADDR_NONE)
    {
        std::cout << "Incorrect ip address!" << std::endl;
        close(socket_fq);
        exit(-1);
    }

    n = sendto(socket_fq, send_buf, strlen(send_buf), 0, (sockaddr *)&serverAddr, sizeof(serverAddr));
    if (n < 0)
    {
        std::cout << "send error!" << std::endl;
        close(socket_fq);
    }

    n = recvfrom(socket_fq, recv_buf, BUFFER_SIZE, 0, (sockaddr *)&serverAddr, &len);
    if (n > 0)
    {
        recv_buf[n] = 0;
        std::cout << "receive:" << recv_buf << std::endl;
    }
    else if (n == 0)
    {
        std::cout << "server closed." << std::endl;
    }
    else if (n == -1)
    {
        std::cout << "recvfrom error." << std::endl;
    }
    close(socket_fq);
    return 0;
}