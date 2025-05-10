#include "../include/protocol.h"
#include "../include/utils.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class FileServer {
private:
    SOCKET serverSocket;
    std::map<std::string, std::mutex> fileMutexes;
    std::mutex mutexMapMutex;
    
    std::mutex& getFileMutex(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutexMapMutex);
        return fileMutexes[filename];
    }
    
    bool handleUpload(SOCKET clientSocket, const Packet& request) {
        std::string filename(request.header.filename);
        std::lock_guard<std::mutex> lock(getFileMutex(filename));
        
        // 创建临时文件
        std::string tempFile = filename + ".tmp";
        uint64_t offset = request.header.offset;
        uint32_t totalSize = request.header.total_size;
        
        // 发送ACK确认开始传输
        if (!NetworkUtils::sendAck(clientSocket, request.header.seq_num)) {
            return false;
        }
        
        // 接收文件数据
        uint64_t receivedSize = 0;
        uint32_t seqNum = 1;
        
        while (receivedSize < totalSize) {
            Packet dataPacket;
            if (!NetworkUtils::receivePacket(clientSocket, dataPacket)) {
                return false;
            }
            
            if (dataPacket.header.type != DATA || dataPacket.header.seq_num != seqNum) {
                delete[] dataPacket.data;
                continue;
            }
            
            // 写入文件
            if (!FileUtils::writeFileChunk(tempFile, dataPacket.data,
                                         offset + receivedSize, dataPacket.header.data_size)) {
                delete[] dataPacket.data;
                return false;
            }
            
            receivedSize += dataPacket.header.data_size;
            delete[] dataPacket.data;
            
            // 发送ACK
            if (!NetworkUtils::sendAck(clientSocket, seqNum)) {
                return false;
            }
            
            seqNum++;
        }
        
        // 接收MD5校验
        Packet md5Packet;
        if (!NetworkUtils::receivePacket(clientSocket, md5Packet) ||
            md5Packet.header.type != MD5_CHECK) {
            delete[] md5Packet.data;
            return false;
        }
        
        std::string receivedMD5(md5Packet.data, md5Packet.header.data_size);
        delete[] md5Packet.data;
        
        // 验证MD5
        if (!FileUtils::verifyMD5(tempFile, receivedMD5)) {
            NetworkUtils::sendError(clientSocket, MD5_MISMATCH, "MD5校验失败");
            return false;
        }
        
        // 重命名临时文件
        if (rename(tempFile.c_str(), filename.c_str()) != 0) {
            NetworkUtils::sendError(clientSocket, PERMISSION_DENIED, "无法保存文件");
            return false;
        }
        
        return true;
    }
    
    bool handleDownload(SOCKET clientSocket, const Packet& request) {
        std::string filename(request.header.filename);
        std::lock_guard<std::mutex> lock(getFileMutex(filename));
        
        if (!FileUtils::fileExists(filename)) {
            NetworkUtils::sendError(clientSocket, FILE_NOT_FOUND, "文件不存在");
            return false;
        }
        
        uint64_t fileSize = FileUtils::getFileSize(filename);
        uint64_t offset = request.header.offset;
        
        if (offset >= fileSize) {
            NetworkUtils::sendError(clientSocket, INVALID_REQUEST, "无效的文件偏移量");
            return false;
        }
        
        // 发送文件信息
        Packet infoPacket;
        infoPacket.header.version = PROTOCOL_VERSION;
        infoPacket.header.type = DATA;
        infoPacket.header.seq_num = 0;
        infoPacket.header.data_size = 0;
        infoPacket.header.offset = offset;
        infoPacket.header.total_size = fileSize;
        strncpy(infoPacket.header.filename, filename.c_str(), sizeof(infoPacket.header.filename) - 1);
        infoPacket.data = nullptr;
        
        if (!NetworkUtils::sendPacket(clientSocket, infoPacket)) {
            return false;
        }
        
        // 等待客户端确认
        if (!NetworkUtils::waitForAck(clientSocket, 0)) {
            return false;
        }
        
        // 发送文件数据
        uint32_t seqNum = 1;
        uint64_t remainingSize = fileSize - offset;
        char buffer[MAX_DATA_SIZE];
        
        while (remainingSize > 0) {
            uint32_t chunkSize = std::min(remainingSize, (uint64_t)MAX_DATA_SIZE);
            
            if (!FileUtils::readFileChunk(filename, buffer, offset, chunkSize)) {
                return false;
            }
            
            Packet dataPacket;
            dataPacket.header.version = PROTOCOL_VERSION;
            dataPacket.header.type = DATA;
            dataPacket.header.seq_num = seqNum;
            dataPacket.header.data_size = chunkSize;
            dataPacket.header.offset = offset;
            dataPacket.header.total_size = fileSize;
            strncpy(dataPacket.header.filename, filename.c_str(), sizeof(dataPacket.header.filename) - 1);
            dataPacket.data = buffer;
            
            if (!NetworkUtils::sendPacket(clientSocket, dataPacket)) {
                return false;
            }
            
            if (!NetworkUtils::waitForAck(clientSocket, seqNum)) {
                return false;
            }
            
            offset += chunkSize;
            remainingSize -= chunkSize;
            seqNum++;
        }
        
        // 发送MD5
        std::string md5 = FileUtils::calculateMD5(filename);
        Packet md5Packet;
        md5Packet.header.version = PROTOCOL_VERSION;
        md5Packet.header.type = MD5_CHECK;
        md5Packet.header.seq_num = seqNum;
        md5Packet.header.data_size = md5.length();
        md5Packet.data = new char[md5.length()];
        memcpy(md5Packet.data, md5.c_str(), md5.length());
        
        bool result = NetworkUtils::sendPacket(clientSocket, md5Packet);
        delete[] md5Packet.data;
        return result;
    }
    
    void handleClient(SOCKET clientSocket) {
        while (true) {
            Packet request;
            if (!NetworkUtils::receivePacket(clientSocket, request)) {
                break;
            }
            
            bool success = false;
            switch (request.header.type) {
                case REQ_UPLOAD:
                    success = handleUpload(clientSocket, request);
                    break;
                case REQ_DOWNLOAD:
                    success = handleDownload(clientSocket, request);
                    break;
                default:
                    NetworkUtils::sendError(clientSocket, INVALID_REQUEST, "无效的请求类型");
                    break;
            }
            
            if (!success) {
                break;
            }
        }
        
        closesocket(clientSocket);
    }
    
public:
    FileServer(int port) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("无法初始化Winsock");
        }
#endif
        
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("无法创建套接字");
        }
        
        int opt = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            closesocket(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("无法设置套接字选项");
        }
        
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        if (bind(serverSocket, (struct sockaddr*)&address, sizeof(address)) < 0) {
            closesocket(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("无法绑定端口");
        }
        
        if (listen(serverSocket, 5) < 0) {
            closesocket(serverSocket);
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("无法监听端口");
        }
    }
    
    void start() {
        std::cout << "服务器已启动，等待连接..." << std::endl;
        
        while (true) {
            struct sockaddr_in clientAddr;
            int clientLen = sizeof(clientAddr);
            SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
            
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "接受连接失败" << std::endl;
                continue;
            }
            
            std::cout << "接受来自 " << inet_ntoa(clientAddr.sin_addr) 
                      << ":" << ntohs(clientAddr.sin_port) << " 的连接" << std::endl;
            
            std::thread clientThread(&FileServer::handleClient, this, clientSocket);
            clientThread.detach();
        }
    }
    
    ~FileServer() {
        closesocket(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "用法: " << argv[0] << " <端口号>" << std::endl;
        return 1;
    }
    
    try {
        int port = std::stoi(argv[1]);
        FileServer server(port);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 