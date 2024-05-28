#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 256
#define MAX_CLIENTS 5

// 错误处理函数
void error(const char *msg) {
  perror(msg); // 打印错误信息
  exit(1);     // 退出程序
}

int sigint_flag = 0; // 标记是否收到SIGINT信号

void handle_sigint(int sig) {
  printf("[srv] SIGINT is coming!\n");
  sigint_flag = 1;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  struct sigaction sa;
  sa.sa_flags = 0;
  sa.sa_handler = handle_sigint;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGINT, &sa, NULL) < 0) {
    perror("sigaction");
    exit(1);
  }

  int sockfd, newsockfd, n; // 定义套接字描述符和读写计数器
  socklen_t clilen;         // 定义客户端地址的长度
  char buffer[BUFFER_SIZE]; // 定义缓冲区
  struct sockaddr_in serv_addr, cli_addr; // 定义服务器和客户端的地址结构

  // 获取绑定的ip
  char *ip = argv[1];
  // 获取绑定的端口
  int PORT = atoi(argv[2]);

  sockfd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字
  if (sockfd < 0)
    error("ERROR opening socket"); // 如果创建失败，打印错误信息并退出

  bzero((char *)&serv_addr, sizeof(serv_addr)); // 清空服务器地址结构
  serv_addr.sin_family = AF_INET;               // 设置地址类型为IPv4
  serv_addr.sin_addr.s_addr = inet_addr(ip);    // 设置服务器的IP地址
  serv_addr.sin_port = htons(PORT); // 设置服务器监听的端口

  // 绑定套接字到服务器的地址和端口
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding"); // 如果绑定失败，打印错误信息并退出

  printf("[srv] server[%s:%d] is initializing!\n", ip, PORT);

  while (!sigint_flag) {
    listen(sockfd, MAX_CLIENTS); // 开始监听端口，等待客户端的连接
    clilen = sizeof(cli_addr);   // 获取客户端地址的长度
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                       &clilen); // 接受客户端的连接

    if (newsockfd < 0) {
      if (errno == EINTR) {
        printf("[srv] listenfd is closed!\n");
        printf("[srv] server is going to exit!\n");
        close(sockfd); // 关闭原始套接字
        exit(0);
      } else
        error("ERROR on accept"); // 如果接受失败，打印错误信息并退出
    }

    // 输出客户端信息
    printf("[srv] client[%s:%d] is accepted!\n", inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));
    bzero(buffer, BUFFER_SIZE); // 清空缓冲区
    // 读取客户端发送的消息,
    n = read(newsockfd, buffer, BUFFER_SIZE - 1);

    n = write(newsockfd, buffer, strlen(buffer)); // 将欢迎消息写入到新的套接字
    if (n < 0)
      error("ERROR writing to socket"); // 如果写入失败，打印错误信息并退出

    close(newsockfd); // 关闭新的套接字
  }
  printf("[srv] listenfd is closed!\n");
  printf("[srv] server is going to exit!\n");
  close(sockfd); // 关闭原始套接字
  return 0;
}