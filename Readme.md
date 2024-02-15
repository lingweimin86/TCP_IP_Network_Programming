《TCP/IP 网络编程》 尹圣雨

# Chapter 1 理解网络编程和套接字

### TCP 一般流程
#### on Linux
```c
#include <sys/socket.h>
int socket (int domain, int type, int protocol);
int bind   (int sockfd, struct sockaddr* myaddr, socklen_t addrlen);
int listen (int sockfd, int backlog);
int accept (int sockfd, struct sockaddr* addr, socklen_t addrlen);
int connect(int sockfd, struct sockaddr* serv_addr, socklen_t addrlen); // client side

int close  (int fd);
```


#### on Windows
ws2_32.lib importing is required.
```c
#include <winsock2.h>

int    WSAStartup (WORD wVersionRequested, LPWSADATA lpWSAData);
SOCKET socket     (int af, int type, int protocol);
int    bind       (SOCKET s, const struct sockaddr* name, int namelen);
int    listen     (SOCKET s, int backlog);
SOCKET accept     (SOCKET s, struct sockaddr* addr, int* addrlen);
int    connect    (SOCKET s, const struct sockaddr* name, int namelen); // client side 
int    closesocket(SOCKET s);
int    WSACleanup (void);
```

# Chapter 2 套接字类型与协议设置

#### SOCK_STREAM 
* 传输的数据不会丢失
* 按序传输
* 不存在数据边界


#### SOCK_DGRAM
* 强调快速传输
* 传输的数据可能丢失也可能损毁
* 有数据边界
* 限制每次传输的数据大小

# Chapter 3 地址族与数据序列

### 四类地址
|分类|字节 |首字节范围|| 
|----|----|----|----| 
| A类 | 1字节网络ID，3字节主机ID|0~127|首位以0开始| 
| B类 | 2字节网络ID，2字节主机ID|128~191|前2位以10开始| 
| C类 | 3字节网络ID，1字节主机ID|192~223|前3位以110开始| 
| D类 | 多播IP地址|0~127|首位以0开始| 


### 网络字节序列

网络字节序列均为**大端**字节序

```c
unsigned short htons (unsigned short); // host byte order to network byte order.
unsigned short ntohs (unsigned short); // network byte order to host byte order.
unsigned long  htonl (unsigned long);
unsigned long  ntohl (unsigned long);

```
字符串信息转换为网络字符序列的整形
```c
#include <arpa/inet.h>
in_addr_t inet_addr(const char * string); 

int       inet_aton(const char * string,, struct in_addr * addr); // return 1 if succeed else 0
char *    inet_ntoa(struct in_addr adr);
```

# Chapter 4 基于TCP的服务器端/客户端(1)


# Chapter 5 基于TCP的服务器端/客户端(2)


# Chapter 6 基于UDP的服务器端/客户端

### UDP 一般流程

#### Server Side
socket -> bind -> recvfrom/sendto -> close
#### Client Side 
socket -> recvfrom/sendto -> close

#### Q. UDP 客户端何时分配IP和端口号
在第一次sendto时由操作系统自动分配端口号

### 已连接(connected)UDP套接字

#### 通过sendto函数传输数据的过程可分为以下3个阶段：
1. 向UDP套接字注册目标IP和端口号
2. 传输数据
3. 删除UDP套接字中注册的目标地址信息

#### 创建已连接UDP套接字
通过调用connect函数创建已连接UDP套接字，之后可使用read/write 替代recvfrom/sendto, 并在传输过程中省略了以上第1和第3步，效率更高。

socket -> connect -> read/write -> close


# Chapter 7 优雅地断开套接字连接

### 半关闭
只关闭输入流或输出流
```c
int shutdown(int sock, int howto);
```
#### howto 选项 (linux)
* SHUT_RD: 断开输入流
* SHUT_WR：断开输出流
* SHUT_RDWR：同时断开输入输出流

# Chapter 8 域名及网络地址


### gethostbyname & gethostbyaddr
```c
#include <netdb.h>
struct hostent * gethostbyname(const char * hostname);
struct hostent * gethostbyaddr(const char * addr, socklen_t len, int family);

struct hostent
{
    char *  h_name;      //official name
    char ** h_aliases;   // alias list
    int     h_addrtype;  // host address type
    int     h_length;    // address length
    char ** h_addr_list; // address list
}

```

# Chapter 9 套接字的多种可选项

|协议层|选项名| 
|-----|------|
|SOL_SOCKET|SO_SNDBUF, SO_RCVBUF, SO_REUSEADDR, SO_TYPE...| 
|IPPROTO_IP|| 
|IPPROTO_TCP|TCP_NODELAY...|  


### getsockopt & setsockopt
```c
int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
int setsockopt(int sock, int level, int optname, void *optval, socklen_t optlen);

```

### SO_SNDBUF & SO_RCVBUF
获取或设置输入输出缓冲区大小

### Time-wait
先发送FIN消息的一方会进入Time-wait. Time-wait的时间为几分钟。SO_REUSEADDR的默认值为0，这就意味着无法分配Time-wait状态下的套接字端口号。因些需要将这个值改为1。

```c
int option = True;
setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&option, sizeof(option));
```

### TCP_NODELAY

#### Nagle算法
只有收到前一个数据的ACK消息时，才发送下一数据

#### 禁用Nagle算法
在无需要等待ACK发送下一个消息的情况可考虑禁用些选项。例如传送大文件时。不使用Nagle算法可提高传输速度。将**TCP_NODELAY**设置为1即可禁用些选项。

# Chapter 10 多进程服务器端 (Linux Only)

### 创建子进程
```c
#include <unistd.h>
pid_t fork(void); 
//1) return >0 for child process Id    
//2) return 0 for parent process
//3) return -1 when fails
```

### 等待子进程退出 (销毁僵尸进程)

```c
#include <sys/wait.h>
pid_t wait(int * statloc); //return child process if success, otherwise return -1
//**NOTE** the function will be blocking till child process ends. If you want no block, use waitpid

//can get the informations from statloc with the macros below

WIFEXITED // return true if child process terminate successfully
WEXITSTATUS // get the return value of child process

```
```c
#include <sys/wait.h>
pid_t waitpid(pid_t pid, int * statloc, int options);

// Parameters:
// pid     -- pass the child process id. pass -1 for any child process.
// statloc -- same meanings as the one in wait
// options -- pass WNOHANG for non-block
// 
// Returns:
// return child process id if succeeds, return -1 if fails
// return 0 if child process is still running
```
### Signal

```c

#include <signal.h>
void (*signal(int signo, void (*func)(int)))(int);

// Parameters: int, void (*func)(int)
// Return: void (*func)(int)

//example
signal(SIGALRM, timeout);

void timeout(int sig)
{
}
```

### sigaction
```c

int sigaction(int signo, const struct sigaction * act, struct sigaction * oldact);

struct sigaction
{
    void (*sa_handler)(int);
    sigset_t sa_mask;
    int sa_flags;
}

//example
struct sigaction act;
act.sa_handler = timeout;
sigemptyset(&act.sa_mask);
act.sa_flags = 0;

sigaction(SIGALRM, &act, 0);
```


# Chapter 11 进程间通信 (Linux Only)

### 管道 Pipe

# Chapter 12 I/O 复用
### select 

# Chapter 13 多种I/O 函数

# Chapter 14 多播与广播

# Chapter 15 套接字与标准I/O

# Chapter 16 关于I/O流分离的其他内容

# Chapter 17 优于select的 epoll

### select 的缺点
* 调用select 函数后常见的针对所有文件描述符的循环语句
* 每次调用select 函数时都需要向该函数传递监视对象信息

### select 也有优点
* 服务器端接入者少时性能影响不大，可使用
* 程序具有兼容性 （Windows 使用IOCP，Linux 使用epoll)

### epoll
* epoll_create - 创建保存epoll文件描述符的空间
* epoll_ctl - 向空间注册并注销文件描述符
* epoll_wait - 与select函数类似，等待文件描述符发生变化


```c
#include <sys/epoll.h>
struct epoll_event
{
    __uint32_t events;
    epoll_data_t data;
}

typedef union epoll_data
{
    void * prt;
    int fd;
    __uint32_t u32;
    __uint64_t u64;
} epoll_data_t



int epoll_create(int size); // return a fd of epoll when success, otherwise return -1.
//NOTE: OS ignores the passed size for Linux version > 2.6.8

int epoll_ctl(int epfd, int op, int fd, struct epoll_event * event); // return 0 when success, else return -1.
// op - EPOLL_CTL_ADD, EPOLL_CTL_DEL, EPOLL_CTL_MOD

int epoll_wait(int epfd, struct epoll_event * events, int maxevents, int timeout);
// timeout - miliseconds, or passing -1 for no timeout
```

### fcntl

```c
#include <fcntl.h>

int fcntl(int fd, int cmd, ... /*arg*/); //return 

//example
int flag = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flag|O_NONBLOCK);
```

### 条件触发和边缘触发

* 条件触发 Level Trigger -- 只要满足条件则一直触发
* 边缘触发 Edge Trigger  -- 只会触发一次

边缘触发的好处 -- 可以分离接受数据和处理数据的时间点

# 编程练习题
### 5.5
### 5.6
### 10.3
### 10.5
### 17.7











