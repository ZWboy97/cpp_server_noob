#include <iostream>
#include <unistd.h> // fork
#include <stdlib.h> // for session
#include <fcntl.h>  // for open
void daemonize()
{
    int fd;
    pid_t pid;    // 子进程的进程pid
    pid = fork(); // 当前进程创建一个子进程，pid为子进程的id
    if (pid < 0)
    {
        std::cout << "can't create suprocess!" << std::endl;
    }
    else
    {
        if (pid != 0) // 判断子进程是否创建成功，为0则创建成功，非0则为父进程
        {
            exit(0); // 父进程退出，子进程成为孤儿进程，被init进程接管
        }
    }
    // 设置sid，建立进程会话
    setsid();
    // 切换工作目录到根目录
    if (chdir("/") < 0)
    {
        std::cout << "can't change dir!" << std::endl;
        exit(-1);
    }
    // 将标准输入、输出、错位重定向到根目录
    fd = open("/dev/null", O_RDWR); // O_WRWR，可读可写
    dup2(fd, STDIN_FILENO);         // 重定向方法，dup2()
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    return;
}

int main(int argc, char *argcs[])
{
    daemonize(); // 调取这个程序之后，就将程序切换到后台
    while (1)
    {
        sleep(1);
    }
    return 0;
}