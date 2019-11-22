## 通过fork实现高性能服务器
##### 原理
- 基于多进程
- 每收到一个连接就fork一个子进程
- 父进程负责接收连接
- fork的子进程负责处理

##### fork的机制
> [从一道面试题谈linux下fork的运行机制](https://kb.cnblogs.com/page/76622/)

- 进程可以看作程序的一次执行，pid标识进程，init的进程pid为1。
- linux使用进程表来存储当前正在运行的程序。
- 进程在内存中呈现树状态，init为根节点，其启动的进程为其子进程，子进程可创建子进程
- fork的作用是复制一个与当前一样的进程，复制的内容包括所有的数据（变量，变量环境，程序计数器）。
- fork将程序分成上下两个部分，父进程和子进程具有相同的数据，一同继续向下执行。
- 根据pid=fork()，pid的值来判断是父进程还是子进程
    - 执行到pid=fork()之后，父进程与子进程在后续执行中pid的值不同
    - 在父进程中，pid为新创建的子进程的pid，非0
    - 在子进程中，pid为0

##### demo
```c++
int pid;
while(1){
    accept_fd = accept(sock...);// socket的acept，接收消息，无消息时候阻塞
    // 当收到消息的时候，启动一个子进程处理
    pid = fork();
    if(pid == 0) // pid 为0 ，子进程执行如下语句
    {
        // 子进程中处理消息
    }
    if(pid != 0){
        // 父进程执行
    }
}
```

##### fork方式的不足和问题
- 进程资源被长期占用
- 分配子进程的开销很大，花费时间长


### 通过select实现高性能服务器
##### 原理
> [I/O多路转换之select与select服务器](https://blog.csdn.net/LLZK_/article/details/62237022)

- 与fork不同，select基于异步I/O，性能上select要更高
- select()同时监控多个文件描述符，一旦其中一个进入就绪状态，就进行IO操作
- select()通过参数readfds,writefds,execptfds将要监控的文件描述符写入，就可以监控这些文件描述符


##### 异步IO
- 一次IO分为两部分（等待数据就绪+进行IO），异步的方式，减少等的比重，增加进行IO部分的比重
- 以事件触发的机制来对IO操作进行处理
- 相比于基于多线程或多进程技术，异步IO优势是不需要创建进程，系统开销小。

##### 思路
- select遍历文件描述符集中的所有描述符，找出有变化的描述符，并对其进行处理
- 对于侦听的socket和数据处理的socket需要区别对待
- socket必须设置为非阻塞方式工作

##### 不足
- 每次进行select都需要将fd集合从用户态拷贝到内核态，开销较大
- select服务器，内部需要不断对fd集进行遍历，当fd很多时候，开销很大
- select服务器的fd集数量有限，一般为1024
- select是半自动的，无法单独确定是哪一个fd准备就绪了，需要遍历fd_sets

##### API
- fd_set
    - 以位图的方式来表示文件描述符的状态
    - 每个bit表示一个fd，一共1024个，所有select最多支持1024个连接
    - select执行之后，系统会修改fd_set中的内容（准备就绪的fd对于的位设置为1）
- 几个宏方法，可以操做fd_set
    - FD_ZERO: 将fd_set清空,全设置为0
    - FD_SET：加入到fd_set 
    - FD_SISET：判断是否在文件描述符集
    - FD_CLEAR：从文件描述符集去除
- flag fcntl(...): 文件描述符的控制函数，设置fd是阻塞还是非阻塞的
- events select(...)

```c++
#include<sys/time.h>
#include<sys/types.h>
#include<unistd.h>
int select(int nfds,fd_set *readfds,fd_set *writefds,fd_set *execptfds,struct timeval *timeout)
// nfds：最大文件描述符+1，用来表示文件描述符的范围,避免对fd表完整拷贝，提高效率
// 参数readfds,writefds,execptfds既是输入参数，又是输出参数。
// 输入：将要监控的文件描述符传给select
// 输出：将处于就绪状态的文件描述符返回。 
// 所以要在每次处理完一就绪事件后要将readfds,writefds,execptfd三个参数重置）
// 返回值：-1 —– 执行错误，0 —– timeout时间到达，其他 —– 正确执行，并且有就绪事件到达
```

##### demo
```c++
#include <iostream>
#include <sys/socket.h>  // for socket(),setsockopt
#include <netinet/in.h>  // for struct sockaddr_in
#include <stdlib.h>      // for exit()
#include <unistd.h>      // for close()
#include <string.h>      // for bzero()
#include <fcntl.h>       // for fcntl()
#define PORT 8881        // tcp 端口
#define MESSAGE_LEN 1024 // 消息大小
#define FD_SIZE 1024     // fd集大小
int main(int argc, char *argv[])
{
    // 结果
    int ret = -1;
    // 连接和发送的fd
    int socket_fd = -1;
    // 用于接收的fd
    int accept_fd = -1;
    //缓冲长度，与并发量相关
    int backlog = 10;
    // socket连接的本地地址以及client端的地址
    struct sockaddr_in localaddr, remoteaddr;
    // setsockopt的参数
    int on = 1;
    // select返回
    int events = 0;
    // 最大的fd，select的参数，为了避免将整个fd集载入
    int max_fd = -1;
    // 接收fd池中的空槽索引
    int curpos = -1;
    // accept_fds中被分配的最大index，目的是减小遍历次数，否则就要遍历FD_SIZE次
    int maxpos = 0;
    // 文件描述符集，select遍历的fd集
    fd_set fd_sets;
    // 用于接收的fd池，默认每个fd为-1，交给select来监听
    int accept_fds[FD_SIZE] = {-1}; // FD_SIZE=1024，所以最多支持1024个连接

    //step1: 创建socket
    socket_fd = socket(PF_INET, SOCK_STREAM, 0); // ipv4，TCP连接
    if (socket_fd == -1)
    {
        std::cout << "failed to create socket!" << std::endl;
        exit(-1);
    }
    // 套接字参数，REUSEADDR地址复用
    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                     &on, sizeof(on));
    if (ret == -1)
    {
        std::cout << "failed to set socket options!" << std::endl;
    }
    // 将socket设置为非阻塞，select要求
    int flags = fcntl(socket_fd, F_GETFL, 0);      // 获取flags
    fcntl(socket_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
    // step2： bind（）
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
    // step3： listen（）
    ret = listen(socket_fd, backlog);
    if (ret == -1)
    {
        std::cout << "failed to listen " << std::endl;
        exit(-1);
    }
    // 初始化
    max_fd = socket_fd;
    // 将连接fd集初始化，之后交给select监听
    for (int i = 0; i < FD_SIZE; i++)
    {
        accept_fds[i] = -1;
    }
    // 进入server 服务循环
    while (1)
    {
        FD_ZERO(&fd_sets);           // 清空fd集
        FD_SET(socket_fd, &fd_sets); // 将socket_fd加入到fd集合中，之后交给select监听事件
        // 遍历accept_fdss，找到有变化的accept_fd，添加到fd_sets中
        for (int i = 0; i < maxpos; i++)
        {
            if (accept_fds[i] != -1)
            {
                if (accept_fds[i] > max_fd) // 更新max_fd
                {
                    max_fd = accept_fds[i];
                }
                FD_SET(accept_fds[i], &fd_sets);
            }
        }
        // select，阻塞监听fd_sets中的fd，处理准备就绪的io
        // fd_sets即是输入，也是函数输出，内核态会更改fd_sets的内容，这也是之后需要遍历fd_sets的原因
        events = select(max_fd + 1, &fd_sets, NULL, NULL, NULL);
        if (events < 0)
        {
            std::cout << "failed to use select" << std::endl;
            break;
        }
        else if (events == 0)
        {
            std::cout << "timeout..." << std::endl;
            continue;
        }
        else if (events) // 有个fd就绪了
        {
            // 需要区别对待新的连接和数据
            if (FD_ISSET(socket_fd, &fd_sets)) // socket_fd 在准备就绪的fd_sets，则是连接来了
            {
                std::cout << "new connection!" << std::endl;
                int i;
                for (i = 0; i < FD_SIZE; i++) // 从accpet_fds找到一个空槽，分配一个给新连接
                {
                    if (accept_fds[i] == -1)
                    {
                        curpos = i;
                        std::cout << "allocate pos= !" << curpos << std::endl;
                        break;
                    }
                }
                if (i == FD_SIZE)
                {
                    std::cout << "the connection is full!" << std::endl;
                    continue;
                }
                // 创建accept_fd,添加到accept_fds中，之后交给select监听
                std::cout << "create accept_fd, and add to accept_fds!" << std::endl;
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr *)&remoteaddr, &addr_len);
                int flags = fcntl(accept_fd, F_GETFL, 0);      // 获取flags
                fcntl(accept_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
                accept_fds[curpos] = accept_fd;
                if (curpos + 1 > maxpos)
                {
                    maxpos = curpos + 1;
                }
                if (accept_fd > max_fd)
                {
                    max_fd = accept_fd;
                }
            }

            for (int i = 0; i < maxpos; i++)
            {
                if (accept_fds[i] != -1 && FD_ISSET(accept_fds[i], &fd_sets))
                {
                    char in_buff[MESSAGE_LEN] = {
                        0,
                    };                               // 接收缓冲区
                    memset(in_buff, 0, MESSAGE_LEN); // 清空inbuff
                    int ret = recv(accept_fds[i], (void *)in_buff, MESSAGE_LEN, 0);
                    if (ret == 0) // 说明没数据了
                    {
                        std::cout << "recv finish，end！ " << std::endl;
                        close(accept_fds[i]);
                        accept_fds[i] = -1;
                    }
                    std::cout << "receive:" << in_buff << std::endl;
                    // step6： return send
                    send(accept_fds[i], (void *)in_buff, MESSAGE_LEN, 0);
                }
            }
        }
    }
    std::cout << "quit server" << std::endl;
    close(socket_fd);
    return 0;
}
```
## 通过epoll+异步IO实现高性能网络服务器
> [Epoll 知乎提问](https://www.zhihu.com/question/20122137/answer/14049112)

##### Epoll的好处
- 没有文件描述符限制，无1024这个限制
- 工作效率不会随着fd的增加而下降，（select中fd增加）
- epoll经过系统优化更高效

##### Epoll的几个API
- epoll_create(): 创建一个epoll对象，epollfd = epoll_create()
- epoll_ctl(): 往epoll对象中增加/删除某一个流的某一个事件比如
    - epoll_ctl(epollfd, EPOLL_CTL_ADD, socket, EPOLLIN),S缓冲区内有数据时epoll_wait返回
    - epoll_ctl(epollfd, EPOLL_CTL_DEL, socket, EPOLLOUT);//缓冲区可写入时epoll_wait返回
- epoll_wait(epollfd,...)等待直到注册的事件发生
- Epoll操作
    - EPOLL_CTL_ADD:
    - EPOLL_CTL_MOD:
    - EPOLL_CTL_DEL:
- Epoll事件
    - EPOLLET：水平触发与边缘触发的设置
    - EPOLLIN：数据来事件
    - EPOLLOUT：数据写事件
    - EPOLLERR:错误事件
    - EPOLLHUP:挂起事件

##### demo
```c++
#include <iostream>
#include <sys/socket.h>  // for socket(),setsockopt
#include <netinet/in.h>  // for struct sockaddr_in
#include <stdlib.h>      // for exit()
#include <unistd.h>      // for close()
#include <string.h>      // for bzero()
#include <fcntl.h>       // for fcntl()
#include <sys/epoll.h>   // for epoll
#include <errno.h>       // for errno
#define PORT 8881        // tcp 端口
#define MESSAGE_LEN 1024 // 消息大小
#define FD_SIZE 20       // epoll event 大小
#define MAX_EVENTS 20
#define TIMEOUT 500
int main(int argc, char *argv[])
{
    int ret = -1;
    int socket_fd = -1;
    int accept_fd = -1;
    int backlog = 10;
    struct sockaddr_in localaddr, remoteaddr;
    int on = 1;
    // epoll的fd
    int epoll_fd;
    // 向epoll中添加fd以及其event
    struct epoll_event event;
    // epoll返回的事件数目
    int event_number;
    //epoll返回的事件列表
    struct epoll_event events[FD_SIZE];

    /*
    * 创建socket的部分与select相同，略
    */

    // 创建epoll
    epoll_fd = epoll_create(256);
    // 将socket_fd的事件添加到epoll中
    event.events = EPOLLIN;
    event.data.fd = socket_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event); // 将侦听的事件添加

    // 进入server 服务循环
    while (1)
    {
        // 阻塞，返回事件数目，返回的events列表，MAX_EVENTS，最大返回的事件数目，TIMEOUT,超时时间
        event_number = epoll_wait(epoll_fd, events, MAX_EVENTS, TIMEOUT);
        for (int i = 0; i < event_number; i++)
        {
            if (events[i].data.fd == socket_fd) // 是新的连接
            {
                // 创建accept_fd,添加到accept_fds中，之后交给select监听
                std::cout << "new connection, create accept_fd!" << std::endl;
                socklen_t addr_len = sizeof(struct sockaddr);
                accept_fd = accept(socket_fd, (struct sockaddr *)&remoteaddr, &addr_len);
                // Epoll也需要设置为非阻塞
                int flags = fcntl(accept_fd, F_GETFL, 0);      // 获取flags
                fcntl(accept_fd, F_SETFD, flags | O_NONBLOCK); // 设置为非阻塞
                // 将其添加到epoll中去
                event.events = EPOLLIN || EPOLLET; // 入，边缘触发(数据不必必须读完)
                event.data.fd = accept_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, accept_fd, &event);
            }
            else if (events[i].events & EPOLLIN) // 输入事件
            {
                char in_buff[MESSAGE_LEN] = {0};
                memset(in_buff, 0, MESSAGE_LEN); // 清空inbuff
                ret = recv(events[i].data.fd, (void *)in_buff, MESSAGE_LEN, 0);
                if (ret == MESSAGE_LEN) // 缓冲区满了,因为设置了边缘触发
                {
                    std::cout << "maybe have data...";
                }
                // 接收缓冲区
                if (ret <= 0)
                {
                    switch (errno)
                    {
                    case EAGAIN: //说明暂时已经没有数据了，要等通知,继续循环
                        break;
                    case EINTR: //被终断了，再接收一次
                        std::cout << "recv EINTR... " << std::endl;
                        ret = recv(events[i].data.fd, &in_buff, MESSAGE_LEN, 0);
                        break;
                    default:
                        std::cout << "the client is closed" << std::endl;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &event);
                        close(events[i].data.fd);
                        break;
                    }
                }
                std::cout << "receive:" << in_buff << std::endl;
                // step6： send back
                send(events[i].data.fd, (void *)in_buff, MESSAGE_LEN, 0);
            }
        }
    }
    std::cout << "quit server" << std::endl;
    close(socket_fd);
    return 0;
}
```

## Epoll + Fork 进一步优化
##### 问题
- epoll是单线程的处理IO
- 目前服务器一般都是多核的
- CPU数 x 2 + 1


## 通过I/O事件处理库实现高性能网络服务器