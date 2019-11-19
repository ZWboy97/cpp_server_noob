#include <iostream>
#include <unistd.h> // fork
#include <stdlib.h> // for session
#include <fcntl.h>  // for open
void daemonize()
{
    /*step1: fork一个子进程*/
    int fd;       // file descriptionor
    pid_t pid;    // 进程id
    pid = fork(); // 当前进程创建一个子进程，pid为子进程的id
    if (pid < 0)
    {
        std::cout << "can't create suprocess!" << std::endl;
        exit(-1);
    }
    else
    {
        if (pid != 0) // 判断子进程是否创建成功，为0则创建成功，非0则为父进程
        {
            exit(0); // 父进程退出，子进程成为孤儿进程，被init进程接管
        }
    }
    /*strp2: 建立新的进程会话*/
    setsid(); //调用setsid来创建新的进程会话。这使得daemon进程成为会话首进程，脱离和terminal的关联。
    /*step3： 切换工作目录到根目录*/
    if (chdir("/") < 0) // 将当前工作目录切换到根目录。父进程继承过来的当前目录可能mount在一个文件系统上
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