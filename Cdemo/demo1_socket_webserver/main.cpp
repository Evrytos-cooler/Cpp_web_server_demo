#include <iostream>      // 标准输入输出流头文件
#include <string>        // 字符串处理
#include <cstring>       // C风格字符串处理
#include <sys/socket.h>  // socket相关API
#include <netinet/in.h>  // sockaddr_in结构体
#include <unistd.h>      // close、read、write等系统调用

// 定义返回给浏览器的HTML内容，包含HTTP响应头和HTML正文
const char* html_response =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n"
    "<html>\n"
    "<head>\n<meta charset=\"utf-8\">\n<title>软件体系架构实验</title>\n</head>\n"
    "<body>\n<h1>软件体系架构实验(1)</h1>\n<p>软件体系架构实验(1), WEB服务器实现</p>\n</body>\n"
    "</html>\n";

int main() {
    // 1. 创建socket，AF_INET表示IPv4，SOCK_STREAM表示TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket"); // 创建失败时输出错误信息
        return 1;
    }

    // 2. 配置服务器地址结构体
    sockaddr_in addr{};                // 初始化为0
    addr.sin_family = AF_INET;         // 使用IPv4
    addr.sin_addr.s_addr = INADDR_ANY; // 监听所有本地IP
    addr.sin_port = htons(8080);       // 端口号8080，htons保证字节序正确

    // 3. 绑定socket到指定IP和端口
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); // 绑定失败
        return 1;
    }

    // 4. 开始监听端口，最多允许5个等待连接
    listen(server_fd, 5);
    std::cout << "服务器已启动，监听8080端口..." << std::endl;

    // 5. 进入主循环，持续接受客户端连接
    while (true) {
        sockaddr_in client_addr{};           // 客户端地址结构体
        socklen_t client_len = sizeof(client_addr);
        // 等待客户端连接，返回新的socket文件描述符
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept"); // 接受连接失败
            continue;
        }
        // 6. 读取客户端请求数据
        char buffer[4096] = {0};             // 存放请求内容的缓冲区
        int len = read(client_fd, buffer, sizeof(buffer)-1); // 读取数据
        if (len > 0) {
            buffer[len] = '\0';             // 确保字符串结尾
            std::cout << "收到请求:\n" << buffer << std::endl; // 打印请求内容
            // 7. 回复HTML内容给客户端
            write(client_fd, html_response, strlen(html_response));
        }
        // 8. 关闭本次客户端连接
        close(client_fd);
    }
    // 9. 关闭服务器socket（理论上不会执行到这里）
    close(server_fd);
    return 0;
}
