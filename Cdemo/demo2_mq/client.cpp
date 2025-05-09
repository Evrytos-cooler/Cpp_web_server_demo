// 包含必要的头文件
#include <iostream>      // 用于输入输出
#include <thread>        // 用于多线程支持
#include <cstring>       // 用于字符串处理
#include <sys/socket.h>  // 用于网络套接字操作
#include <netinet/in.h>  // 用于网络地址结构
#include <arpa/inet.h>   // 用于IP地址转换
#include <unistd.h>      // 用于系统调用

// Windows系统特定的头文件
#ifdef _WIN32
#include <windows.h>
#endif

// 接收消息的线程函数
void recv_thread(int sockfd) {
    char buffer[1024];  // 定义接收缓冲区
    while (true) {
        // 从服务器读取数据
        int len = read(sockfd, buffer, sizeof(buffer)-1);
        if (len <= 0) break;  // 如果读取失败或连接关闭，退出循环
        buffer[len] = '\0';   // 确保字符串正确终止
        std::cout << "收到: " << buffer << std::endl;  // 打印接收到的消息
    }
}

int main() {
    // Windows系统下设置控制台输出为UTF-8编码
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    // 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");  // 如果创建失败，打印错误信息
        return 1;
    }

    // 设置服务器地址结构
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;           // 使用IPv4
    server_addr.sin_port = htons(9000);         // 设置端口号（9000）
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);  // 设置服务器IP地址（本地回环地址）

    // 连接到服务器
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");  // 如果连接失败，打印错误信息
        return 1;
    }

    // 创建接收消息的线程
    std::thread t(recv_thread, sockfd);

    // 主循环：发送消息
    std::string msg;
    while (true) {
        std::getline(std::cin, msg);  // 从控制台读取用户输入
        if (msg == "exit") break;     // 如果输入"exit"，退出循环
        write(sockfd, msg.c_str(), msg.size());  // 发送消息到服务器
    }

    // 清理资源
    close(sockfd);  // 关闭套接字
    t.join();       // 等待接收线程结束
    return 0;
}
