
## 后台进程
#### fork方式
- 四个步骤
    - fork 一个子进程，父进程退出了，那么子进程将成为孤儿进程，被init进程接管
    - 调用setsid建立新的进程会话
    - 将当前工作目录切换到更目录（因为父亲进程已经没了，init进程的工作目录在根目录）
    - 将标准输出，输入，出错重定向到 ```/dev/null```
- Demo 代码
```c++
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
```
#### 调用系统的daemon API
- linux的系统函数，其实实际上也是走上边的四个步骤,推荐优先使用
    - unistd.h中的daemon() api
-demo
```c++
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argcs[])
{
    /* Put the program in the background, and dissociate from the controlling
    terminal.  If NOCHDIR is zero, do `chdir ("/")'.  If NOCLOSE is zero,
    redirects stdin, stdout, and stderr to /dev/null.  */
    if (daemon(0, 0) == -1)
    {
        std::cout << "error" << std::endl;
        exit(-1);
    }
    // 以上代码便可以实现将进程切换到后台运行，之后再执行其他语句
    while (1)
    {
        sleep(1);
    }
    return 0;
}
```

