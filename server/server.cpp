/**
* Server class
* @auther ljc
* @data 2019-11-18
*/
#include <iostream> // C++的库
#include <unistd.h> // 引入C语言的相关方法
#include "server.h" // 引入同目录下的头文件

namespace avdance
{

Server::Server()
{
    std::cout << "constructor..." << std::endl;
}

Server::~Server()
{
    std::cout << "destruct..." << std::endl;
}

void Server::run()
{
    while (1)
    {
        std::cout << "running..." << std::endl;
        ::usleep(1000000);
    }
}

} // namespace avdance