#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(0);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);
    int ret = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect");
        exit(0);
    }

    int num = 0;
    // 通信
    while (1)
    {
        char buf[1024];
        //从终端读一个字符
        //fgets(buf, sizeof(buf), stdin);

        sprintf(buf,"hello,world,%d\n...",num++);
        //发数据
        write(fd, buf, strlen(buf));

        //接受
        //阻塞等待
        int len = read(fd,buf,sizeof(buf));
        if(len == -1) {
            perror("read error");
            exit(1);
        }
        printf("read buf = %s\n", buf);
        //每隔一秒发送一次
        sleep(1);
    }
    close(fd);
    return 0;
}