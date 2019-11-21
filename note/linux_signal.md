#### Linux 下的几个重要信号
- SIGPIPE：管道终止信号，当写入无人读取的管道时产生该信号，默认终止进程，需要我们去处理
    - 网络程序必须要处理，否则Client端断开连接之后，会导致Server Crash
- SIGCHLD：子进程结束或者停止发送时候
    - 容易产生僵尸进程（一个早已死亡的进程，但是在进程表中还存在）
    - 子进程结束的时候，他并没有完全销毁，因为父进程还需要使用它的消息。
    - 父进程没有处理SIGCHLD信号，或者没有调用wait/waitpid等待子进程结束，就会出现僵尸进程。
- SIGALRM: 定时信号，秒为单位，默认会终止进程，所以需要我们去处理（捕获，然后忽略）
- SIGINT: 键盘输入的退出信号
- SIGQUIT： 键盘输入的退出信号
- SIGHUP：控制终端挂起信号

## 信号的发送和处理
- 硬件方式
    - ctrl+c，ctrl+\
## 安装信号
#### signal(int sig, void (*func)(int));
```c++
#include <iostream>
#include <signal.h>
#include <unistd.h>

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
```

#### 通过 sigaction方法
- sigaction
```c++
struct sigaction
{
    void (*sa_handler)(int sig); // 捕获到sign的处理函数，需要设置
    void (*sa_sigaction)(int, siginfo_t *, void *); // 与上类似，一般不用
    sigset_tsa_mask; // 掩码，需要设置
    int sa_flags; //根据SA_SIGINFO标记是选择sa_handler还是sigaction进行处理，需要设置
};
```
- Demo
```c++
#include <iostream>
#include <signal.h>
#include <unistd.h>
void sig_handler(int sig)   // 处理函数
{
    std::cout << "receive signal:" << sig << std::endl;
}
int main(int argc, char *argcs[])
{
    struct sigaction act, oact;
    act.sa_handler = sig_handler;
    sigfillset(&act.sa_mask); // 设置掩码
    act.sa_flags = 0;   // 选择使用sighandler来处理
    sigaction(SIGINT, &act, &oact);
    sigaction(SIGQUIT, &act, &oact);
    pause();
    return 0;
}
```