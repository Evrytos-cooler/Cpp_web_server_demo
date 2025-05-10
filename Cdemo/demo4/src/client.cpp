#include "../include/protocol.h"
#include "../include/utils.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class FileClient {
private:
    SOCKET clientSocket;
    std::string serverAddress;
    int serverPort;
    
    bool connect() {
        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);
        inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);
        
        if (::connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            return false;
        }
        
        return true;
    }
    
    bool uploadFile(const std::string& filename, uint64_t offset = 0) {
        if (!FileUtils::fileExists(filename)) {
            std::cerr << "文件不存在: " << filename << std::endl;
            return false;
        }
        
        uint64_t fileSize = FileUtils::getFileSize(filename);
        if (offset >= fileSize) {
            std::cerr << "无效的文件偏移量" << std::endl;
            return false;
        }
        
        // 发送上传请求
        Packet request;
        request.header.version = PROTOCOL_VERSION;
        request.header.type = REQ_UPLOAD;
        request.header.seq_num = 0;
        request.header.data_size = 0;
        request.header.offset = offset;
        request.header.total_size = fileSize;
        strncpy(request.header.filename, filename.c_str(), sizeof(request.header.filename) - 1);
        request.data = nullptr;
        
        if (!NetworkUtils::sendPacket(clientSocket, request)) {
            return false;
        }
        
        // 等待服务器确认
        if (!NetworkUtils::waitForAck(clientSocket, 0)) {
            return false;
        }
        
        // 发送文件数据
        uint32_t seqNum = 1;
        uint64_t remainingSize = fileSize - offset;
        char buffer[MAX_DATA_SIZE];
        uint64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
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
            
            // 显示进度
            uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            uint64_t elapsedTime = currentTime - startTime;
            uint64_t uploadedSize = offset;
            double speed = (double)uploadedSize / (elapsedTime / 1000.0);
            
            std::cout << "\r上传进度: " << (uploadedSize * 100 / fileSize) << "% "
                      << "(" << FileUtils::formatSize(uploadedSize) << "/" 
                      << FileUtils::formatSize(fileSize) << ") "
                      << FileUtils::formatSpeed(speed) << "     " << std::flush;
        }
        
        std::cout << std::endl;
        
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
    
    bool downloadFile(const std::string& filename, uint64_t offset = 0) {
        // 发送下载请求
        Packet request;
        request.header.version = PROTOCOL_VERSION;
        request.header.type = REQ_DOWNLOAD;
        request.header.seq_num = 0;
        request.header.data_size = 0;
        request.header.offset = offset;
        request.header.total_size = 0;
        strncpy(request.header.filename, filename.c_str(), sizeof(request.header.filename) - 1);
        request.data = nullptr;
        
        if (!NetworkUtils::sendPacket(clientSocket, request)) {
            return false;
        }
        
        // 接收文件信息
        Packet infoPacket;
        if (!NetworkUtils::receivePacket(clientSocket, infoPacket)) {
            return false;
        }
        
        if (infoPacket.header.type != DATA) {
            delete[] infoPacket.data;
            return false;
        }
        
        uint64_t fileSize = infoPacket.header.total_size;
        delete[] infoPacket.data;
        
        // 发送ACK
        if (!NetworkUtils::sendAck(clientSocket, 0)) {
            return false;
        }
        
        // 创建临时文件
        std::string tempFile = filename + ".tmp";
        uint64_t receivedSize = 0;
        uint32_t seqNum = 1;
        uint64_t startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        while (receivedSize < fileSize) {
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
            
            // 显示进度
            uint64_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
            uint64_t elapsedTime = currentTime - startTime;
            double speed = (double)receivedSize / (elapsedTime / 1000.0);
            
            std::cout << "\r下载进度: " << (receivedSize * 100 / fileSize) << "% "
                      << "(" << FileUtils::formatSize(receivedSize) << "/" 
                      << FileUtils::formatSize(fileSize) << ") "
                      << FileUtils::formatSpeed(speed) << "     " << std::flush;
        }
        
        std::cout << std::endl;
        
        // 接收MD5
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
            std::cerr << "MD5校验失败" << std::endl;
            return false;
        }
        
        // 重命名临时文件
        if (rename(tempFile.c_str(), filename.c_str()) != 0) {
            std::cerr << "无法保存文件" << std::endl;
            return false;
        }
        
        return true;
    }
    
public:
    FileClient(const std::string& address, int port) 
        : serverAddress(address), serverPort(port) {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("无法初始化Winsock");
        }
#endif
        
        clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
#ifdef _WIN32
            WSACleanup();
#endif
            throw std::runtime_error("无法创建套接字");
        }
    }
    
    bool upload(const std::string& filename) {
        if (!connect()) {
            std::cerr << "无法连接到服务器" << std::endl;
            return false;
        }
        
        bool result = uploadFile(filename);
        closesocket(clientSocket);
        return result;
    }
    
    bool download(const std::string& filename) {
        if (!connect()) {
            std::cerr << "无法连接到服务器" << std::endl;
            return false;
        }
        
        bool result = downloadFile(filename);
        closesocket(clientSocket);
        return result;
    }
    
    ~FileClient() {
        closesocket(clientSocket);
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

void printUsage(const char* program) {
    std::cout << "用法: " << program << " <服务器地址> <端口号> <上传/下载> <文件名>" << std::endl;
    std::cout << "命令:" << std::endl;
    std::cout << "  upload <本地文件路径> <远程保存路径>" << std::endl;
    std::cout << "  download <远程文件路径> <本地保存路径>" << std::endl;
    std::cout << "  quit" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        std::string serverAddress = argv[1];
        int port = std::stoi(argv[2]);
        std::string mode = argv[3];
        std::string filename = argv[4];
        
        FileClient client(serverAddress, port);
        
        if (mode == "upload") {
            if (!client.upload(filename)) {
                std::cerr << "上传失败" << std::endl;
                return 1;
            }
            std::cout << "上传成功" << std::endl;
        } else if (mode == "download") {
            if (!client.download(filename)) {
                std::cerr << "下载失败" << std::endl;
                return 1;
            }
            std::cout << "下载成功" << std::endl;
        } else {
            std::cerr << "无效的模式，请使用 'upload' 或 'download'" << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 