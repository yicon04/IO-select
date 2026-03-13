#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>
#include <pthread.h>

//互斥锁 给共享数据加锁：maxfd,rdset 在函数中都是传地址的使用的是同一块内存； 
pthread_mutex_t mutex;

typedef struct fdinfo
{
    int fd;
    int *maxfd;
    fd_set *rdset;
} FDInfo;

void *acceptConn(void *arg)
{
    printf("子线程ID： %ld\n",pthread_self());
    FDInfo *info = (FDInfo *)arg;
    int cfd = accept(info->fd, NULL, NULL);
    pthread_mutex_lock(&mutex);
    FD_SET(cfd, info->rdset);
    *info->maxfd = cfd > *info->maxfd ? cfd : *info->maxfd;
    pthread_mutex_unlock(&mutex);

    free(info);
}

// 通信函数
void *communication(void *arg)
{
    printf("子线程ID： %ld\n",pthread_self());
    FDInfo* info = (FDInfo *)arg;
    char buf[1024] = {0};
    int len = recv(info->fd, buf, sizeof(buf), 0);
    if (len == 0)
    {
        printf("客户端关闭连接...");
        pthread_mutex_lock(&mutex);
        FD_CLR(info->fd, info->rdset);
        pthread_mutex_unlock(&mutex);
        close(info->fd);
        free(info);
        return NULL;
    }
    else if (len == -1)
    {
        perror("recv");
        free(info);
        return NULL;
    }
    printf("read buf = %s\n", buf);
    for (int i = 0; i < len; i++)
    {
        // 小写转为大写
        buf[i] = toupper(buf[i]);
    }
    printf("after buf = %s\n", buf);

    // 大写发给客户端
    int ret = send(info->fd, buf, strlen(buf) + 1, 0);
    if (ret == -1)
    {
        perror("send error");
        exit(1);
    }
    free(info);
    return NULL;
}

int main()
{
    pthread_mutex_init(&mutex,NULL);

    // 1.创建用于监听的套接字
    int lfd = socket(AF_INET, SOCK_STREAM, 0);

    // 2.绑定 IP 端口
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(lfd, (struct sockaddr *)&addr, sizeof(addr));

    // 3.监听
    listen(lfd, 128);

    // 初始化一个读集合
    //主线程中，maxfd rdset在创建时，子线程还没有被创建出来，所以不用加锁
    int maxfd = lfd; // 定义一个变量，用于记录当前最大的文件描述符
    fd_set rdset;
    fd_set rdtemp;
    FD_ZERO(&rdset);
    FD_SET(lfd, &rdset);

    while (1)
    {
        pthread_mutex_lock(&mutex);
        rdtemp = rdset;
        pthread_mutex_unlock(&mutex);
        int ret = select(maxfd + 1, &rdtemp, NULL, NULL, NULL);
        // 判断是否为监听的fd
        if (FD_ISSET(lfd, &rdtemp))
        {
            // 接收客户端的连接 //需要多线程，创建子线程,传入各个线程的fd和maxfd和rdset
            pthread_t tid;
            FDInfo *info = (FDInfo *)malloc(sizeof(FDInfo));
            info->fd = lfd;
            info->maxfd = &maxfd;
            info->rdset = &rdset;
            pthread_create(&tid, NULL, acceptConn, info);
            // 将主线程和子线程进行线程分离
            pthread_detach(tid);
        }

        for (int i = 0; i < maxfd + 1; i++)
        {
            if (i != lfd && FD_ISSET(i, &rdtemp))
            {
                // 接收数据
                pthread_t tid;
                FDInfo *info = (FDInfo *)malloc(sizeof(FDInfo));
                info->fd = i;
                info->rdset = &rdset;
                pthread_create(&tid, NULL, communication, info);
                // 将主线程和子线程进行线程分离
                pthread_detach(tid);
            }
        }
    }
    close(lfd);
    pthread_mutex_destroy(&mutex);
    return 0;
}