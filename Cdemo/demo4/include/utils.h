#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <iomanip>
#include "protocol.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class FileUtils {
public:
    // 文件操作相关函数
    static bool fileExists(const std::string& filename);
    static uint64_t getFileSize(const std::string& filename);
    static bool createDirectory(const std::string& path);
    static std::string getDirectory(const std::string& path);
    static std::string getFileName(const std::string& path);
    
    // 文件读写相关函数
    static bool readFileChunk(const std::string& filename, char* buffer, 
                            uint64_t offset, uint32_t size);
    static bool writeFileChunk(const std::string& filename, const char* buffer,
                             uint64_t offset, uint32_t size);
    
    // MD5相关函数
    static std::string calculateMD5(const std::string& filename);
    static bool verifyMD5(const std::string& filename, const std::string& expected_md5);
    
    // 进度显示相关函数
    static void showProgress(uint64_t current, uint64_t total, const std::string& operation);
    static std::string formatSize(uint64_t size);
    static std::string formatSpeed(uint64_t bytes, double seconds);
};

class NetworkUtils {
public:
    // 网络相关函数
    static bool sendPacket(SOCKET socket, const Packet& packet);
    static bool receivePacket(SOCKET socket, Packet& packet);
    static bool waitForAck(SOCKET socket, uint32_t expected_seq, int timeout_ms = 5000);
    static bool sendAck(SOCKET socket, uint32_t seq_num);
    static bool sendError(SOCKET socket, ErrorCode code, const std::string& message);
};

#endif // UTILS_H 