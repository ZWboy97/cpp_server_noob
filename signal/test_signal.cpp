#include <iostream>
#include <signal.h> // for signal()
#include <unistd.h> // for pause()

void sighandle(int sig)
{
    std::cout << "receive signal:" << sig << std::endl;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sighandle); // 捕获SIGINT信号
    signal(SIGQUIT, sighandle);
    signal(SIGHUP, sighandle);
    pause();
}