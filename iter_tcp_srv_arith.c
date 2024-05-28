#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080  // 定义服务器监听的端口
#define MAX_CLIENTS 5  // 定义服务器最大可以处理的客户端数量
#define BUFFER_SIZE 256  // 定义缓冲区的大小

// 错误处理函数
void error(const char *msg) {
    perror(msg);  // 打印错误信息
    exit(1);  // 退出程序
}

int main() {
    int sockfd, newsockfd, n;  // 定义套接字描述符和读写计数器
    socklen_t clilen;  // 定义客户端地址的长度
    char buffer[BUFFER_SIZE];  // 定义缓冲区
    struct sockaddr_in serv_addr, cli_addr;  // 定义服务器和客户端的地址结构

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // 创建套接字
    if (sockfd < 0) 
       error("ERROR opening socket");  // 如果创建失败，打印错误信息并退出

    bzero((char *) &serv_addr, sizeof(serv_addr));  // 清空服务器地址结构
    serv_addr.sin_family = AF_INET;  // 设置地址类型为IPv4
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // 设置服务器IP地址为本机的任意IP
    serv_addr.sin_port = htons(PORT);  // 设置服务器监听的端口

    // 绑定套接字到服务器的地址和端口
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR on binding");  // 如果绑定失败，打印错误信息并退出

    listen(sockfd, MAX_CLIENTS);  // 开始监听端口，等待客户端的连接
    clilen = sizeof(cli_addr);  // 获取客户端地址的长度
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);  // 接受客户端的连接
    if (newsockfd < 0) 
        error("ERROR on accept");  // 如果接受失败，打印错误信息并退出

    bzero(buffer, BUFFER_SIZE);  // 清空缓冲区
    strcpy(buffer, "Welcome to the server!\n");  // 将欢迎消息复制到缓冲区
    n = write(newsockfd, buffer, strlen(buffer));  // 将欢迎消息写入到新的套接字
    if (n < 0) 
        error("ERROR writing to socket");  // 如果写入失败，打印错误信息并退出

    close(newsockfd);  // 关闭新的套接字
    close(sockfd);  // 关闭原始套接字
    return 0; 
}