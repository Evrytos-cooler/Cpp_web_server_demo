#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

// 协议版本
#define PROTOCOL_VERSION 1

// 数据包类型
enum PacketType : uint8_t {
    REQ_UPLOAD = 1,      // 上传请求
    REQ_DOWNLOAD = 2,    // 下载请求
    DATA = 3,           // 数据包
    ACK = 4,            // 确认包
    ERROR = 5,          // 错误包
    MD5_CHECK = 6,      // MD5校验包
    RESUME = 7          // 断点续传请求
};

// 数据包头部结构（固定长度）
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t version;        // 协议版本
    uint8_t type;          // 数据包类型
    uint32_t seq_num;      // 序列号
    uint32_t data_size;    // 数据部分大小
    uint64_t offset;       // 文件偏移量
    uint32_t total_size;   // 文件总大小
    char filename[256];    // 文件名
};
#pragma pack(pop)

// 数据包结构
struct Packet {
    PacketHeader header;
    char* data;
};

// 错误码定义
enum ErrorCode : uint32_t {
    SUCCESS = 0,
    FILE_NOT_FOUND = 1,
    PERMISSION_DENIED = 2,
    DISK_FULL = 3,
    MD5_MISMATCH = 4,
    NETWORK_ERROR = 5,
    INVALID_REQUEST = 6
};

// 常量定义
constexpr size_t MAX_PACKET_SIZE = 1024 * 1024;  // 最大数据包大小（1MB）
constexpr size_t HEADER_SIZE = sizeof(PacketHeader);
constexpr size_t MAX_DATA_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;

// 辅助函数声明
std::string calculateMD5(const std::string& filename);
bool verifyMD5(const std::string& filename, const std::string& expected_md5);

#endif // PROTOCOL_H 