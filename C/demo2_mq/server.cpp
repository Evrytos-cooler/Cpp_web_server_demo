#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

// 保存所有已连接的客户端socket
std::vector<int> clients;
// 保护clients的互斥锁，保证多线程安全
std::mutex clients_mutex;

// 处理每个客户端连接的线程函数
void handle_client(int client_fd) {
    char buffer[1024];
    while (true) {
        // 读取客户端发来的消息
        int len = read(client_fd, buffer, sizeof(buffer)-1);
        if (len <= 0) break; // 断开连接
        buffer[len] = '\0';
        std::cout << "收到消息: " << buffer << std::endl;
        // 广播消息给其他客户端
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (int fd : clients) {
            if (fd != client_fd) {
                write(fd, buffer, len);
            }
        }
    }
    // 客户端断开，清理资源
    close(client_fd);
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
}

int main() {
    // 创建socket，AF_INET表示IPv4，SOCK_STREAM表示TCP
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }
    // 配置服务器地址
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(9000); // 监听9000端口
    // 绑定socket
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }
    // 开始监听
    listen(server_fd, 5);
    std::cout << "消息服务器已启动，监听9000端口..." << std::endl;
    while (true) {
        // 等待客户端连接
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        // 新客户端加入列表
        {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.push_back(client_fd);
        }
        // 为每个客户端创建独立线程
        std::thread(handle_client, client_fd).detach();
    }
    // 关闭服务器socket
    close(server_fd);
    return 0;
}
