# Socket 编程基础

## 实现一个TCP Server
#### Server端步骤
- socker():创建socket，指定使用TCP协议
- bind():将socket与地址和端口进行绑定
- listen():侦听端口
- accept():创建新的socket
- recv():使用recv接收数据
- send():使用send发送数据
- close():使用close关闭连接
#### TCP常见套接字选项
- SO_REUSEADDR: 地址重用
    - 之前程序结束，端口处于WAIT_TIME状态下，新启动的程序仍然可以启动
- SO——RCVBUF：设置接收缓冲区大小
- SO_SNDBUF: 设置发送缓冲区大小
#### Demo
```c++

```

## 实现一个TCP Client
#### Client端步骤
- socket():创建socket，指定使用TCP协议
- connect():操作系统随机分配一个随机的端口和IP地址
- send():发送
- recv():接收
- close():关闭
#### Demo
```c++


```