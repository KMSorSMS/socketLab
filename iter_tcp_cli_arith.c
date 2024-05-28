#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 160

// 将int64_t类型的数据从主机字节序转换为网络字节序
int64_t htonll(int64_t val) {
    if (htons(1) != 1) {
        val = ((int64_t)htonl(val)) << 32 | htonl(val >> 32);
    }
    return val;
}
// 将int64_t类型的数据从网络字节序转换为主机字节序
int64_t ntohll(int64_t val) {
    if (htons(1) != 1) {
        val = ((int64_t)ntohl(val)) << 32 | ntohl(val >> 32);
    }
    return val;
}

void cli_biz(int sockfd) {
    char buffer[BUFFER_SIZE];
    char op_str[4];
    int64_t op1, op2,op1_h,op2_h;
    int32_t op;
    while (1) {
        // 从stdin读取用户的命令行计算指令
        fgets(buffer, sizeof(buffer), stdin);

        // 检查是否是退出指令
        if (strcmp(buffer, "EXIT\n") == 0) {
            printf("[cli] command EXIT received\n");
            return;
        }
        // 解析指令
        sscanf(buffer, "%s %ld %ld", op_str, &op1, &op2);

        // 根据操作符字符串确定操作符
        if (strcmp(op_str, "ADD") == 0) {
            op = 1;
            op_str[0] = '+';
            op_str[1] = '\0';
        } else if (strcmp(op_str, "SUB") == 0) {
            op = 2;
            op_str[0] = '-';
            op_str[1] = '\0';
        } else if (strcmp(op_str, "MUL") == 0) {
            op = 4;
            op_str[0] = '*';
            op_str[1] = '\0';
        } else if (strcmp(op_str, "DIV") == 0) {
            op = 8;
            op_str[0] = '/';
            op_str[1] = '\0';
        } else if (strcmp(op_str, "MOD") == 0) {
            op = 16;
            op_str[0] = '%';
            op_str[1] = '\0';
        } else {
            fprintf(stderr, "Invalid operation: %s\n", op_str);
            return;
        }

        // 将操作数和操作符转换为网络字节序
        op1_h = op1; // 保存原始数据
        op2_h = op2; // 保存原始数据
        op = htonl(op);
        op1 = htonll(op1); // 64位整数转换为网络字节序
        op2 = htonll(op2); // 64位整数转换为网络字节序

        // 将操作数和操作符写入缓冲区
        memcpy(buffer, &op, sizeof(op));
        memcpy(buffer + sizeof(op), &op1, sizeof(op1));
        memcpy(buffer + sizeof(op) + sizeof(op1), &op2, sizeof(op2));

        // 发送数据到服务器
        int n = write(sockfd, buffer, sizeof(op) + sizeof(op1) + sizeof(op2));
        if (n <= 0) {
            perror("ERROR writing to socket");
            return;
        }

        // 读取服务器的响应
        bzero(buffer, BUFFER_SIZE);
        n = read(sockfd, buffer, BUFFER_SIZE - 1);
        if (n <= 0) {
            perror("ERROR reading from socket");
            return;
        }

        // 将网络字节序转换为主机字节序
        int64_t res;
        memcpy(&res, buffer, sizeof(res));
        res = ntohll(res);

        printf("[rep_rcv] %ld %s %ld = %ld\n", op1_h, op_str, op2_h, res);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in serv_addr;

    // 获取服务器的ip和端口
    char *ip = argv[1];
    int PORT = atoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("ERROR invalid server IP");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR connecting");
        exit(1);
    }
    // 输出服务器连接成功的信息
    printf("[cli] server[%s:%d] is connected!\n", ip, PORT);

    cli_biz(sockfd);
    close(sockfd);
    printf("[cli] connfd is closed!\n");
    printf("[cli] client is going to exit!\n");
    return 0;
}