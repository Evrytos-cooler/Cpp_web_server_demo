#include <iostream>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void recv_thread(int sockfd) {
    char buffer[1024];
    while (true) {
        int len = read(sockfd, buffer, sizeof(buffer)-1);
        if (len <= 0) break;
        buffer[len] = '\0';
        std::cout << "收到: " << buffer << std::endl;
    }
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (connect(sockfd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }
    std::thread t(recv_thread, sockfd);
    std::string msg;
    while (true) {
        std::getline(std::cin, msg);
        if (msg == "exit") break;
        write(sockfd, msg.c_str(), msg.size());
    }
    close(sockfd);
    t.join();
    return 0;
}
