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

#define BUFFER_SIZE 160
#define MAX_CLIENTS 5

// 错误处理函数
void error(const char *msg) {
  printf(",%s\n", msg); // 打印错误信息
  exit(1);              // 退出程序
}

int sigint_flag = 0; // 标记是否收到SIGINT信号

void handle_sigint(int sig) {
  printf("[srv] SIGINT is coming!\n");
  sigint_flag = 1;
}

// 将int64_t类型的数据从网络字节序转换为主机字节序
int64_t ntohll(int64_t val) {
  if (htons(1) != 1) {
    val = ((int64_t)ntohl(val)) << 32 | ntohl(val >> 32);
  }
  return val;
}

// 将int64_t类型的数据从主机字节序转换为网络字节序
int64_t htonll(int64_t val) {
  if (htons(1) != 1) {
    val = ((int64_t)htonl(val)) << 32 | htonl(val >> 32);
  }
  return val;
}

// 服务器业务处理函数
void srv_biz(int sockfd) {
  int n;
  int32_t op;
  int64_t op1, op2, res;
  char buffer[BUFFER_SIZE];
  while (!sigint_flag) {
    // 读取客户端发送的数据
    bzero(buffer, BUFFER_SIZE);
    // 这里每次就只读一个PDU，一次次读取内容
    n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n <= 0)
      return;
    // 将网络字节序转换为主机字节序
    memcpy(&op, buffer, sizeof(op));
    memcpy(&op1, buffer + sizeof(op), sizeof(op1));
    memcpy(&op2, buffer + sizeof(op) + sizeof(op1), sizeof(op2));
    op = ntohl(op);
    op1 = ntohll(op1);
    op2 = ntohll(op2);
    // 进行计算操作
    switch (op) {
    case 1:
      res = op1 + op2;
      printf("[rqt_res] %ld + %ld = %ld\n", op1, op2, res);
      break;
    case 2:
      res = op1 - op2;
      printf("[rqt_res] %ld - %ld = %ld\n", op1, op2, res);
      break;
    case 4:
      res = op1 * op2;
      printf("[rqt_res] %ld * %ld = %ld\n", op1, op2, res);
      break;
    case 8:
      if (op2 == 0) {
        res = -1;
        printf("[rqt_res] %ld / %ld = -1\n", op1, op2);
      } else {
        res = op1 / op2;
        printf("[rqt_res] %ld / %ld = %ld\n", op1, op2, res);
      }
      break;
    case 16:
      res = op1 % op2;
      printf("[rqt_res] %ld %% %ld = %ld\n", op1, op2, res);
      break;
    default:
      res = -1;
      printf("[rqt_res] invalid operator!\n");
      break;
    }
    res = htonll(res);
    n = write(sockfd, &res, sizeof(res));
    if (n < 0)
      error("ERROR writing to socket");
  }
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
  listen(sockfd, MAX_CLIENTS); // 开始监听端口，等待客户端的连接
  clilen = sizeof(cli_addr);   // 获取客户端地址的长度
  while (!sigint_flag) {
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr,
                       &clilen); // 接受客户端的连接
    if (newsockfd < 0) {
      if (errno == EINTR) {
        continue;
      } else
        error("ERROR on accept"); // 如果接受失败，打印错误信息并退出
    }

    // 输出客户端信息
    printf("[srv] client[%s:%d] is accepted!\n", inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));
    // 调用业务函数与客户端进行收发互动
    srv_biz(newsockfd);
    // 输出客户端断开连接的信息
    printf("[srv] client[%s:%d] is closed!\n", inet_ntoa(cli_addr.sin_addr),
           ntohs(cli_addr.sin_port));
    close(newsockfd); // 关闭新的套接字
  }

  printf("[srv] listenfd is closed!\n");
  printf("[srv] server is going to exit!\n");
  close(sockfd); // 关闭原始套接字
  return 0;
}