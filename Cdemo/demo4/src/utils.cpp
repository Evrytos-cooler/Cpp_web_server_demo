#include "../include/utils.h"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <openssl/md5.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

// FileUtils 实现
bool FileUtils::fileExists(const std::string& filename) {
    return fs::exists(filename);
}

uint64_t FileUtils::getFileSize(const std::string& filename) {
    return fs::file_size(filename);
}

bool FileUtils::createDirectory(const std::string& path) {
    return fs::create_directories(path);
}

std::string FileUtils::getDirectory(const std::string& path) {
    return fs::path(path).parent_path().string();
}

std::string FileUtils::getFileName(const std::string& path) {
    return fs::path(path).filename().string();
}

bool FileUtils::readFileChunk(const std::string& filename, char* buffer,
                            uint64_t offset, uint32_t size) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    
    file.seekg(offset);
    file.read(buffer, size);
    return file.good();
}

bool FileUtils::writeFileChunk(const std::string& filename, const char* buffer,
                             uint64_t offset, uint32_t size) {
    // 确保目录存在
    createDirectory(getDirectory(filename));
    
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) {
        file.open(filename, std::ios::binary | std::ios::out);
    }
    
    if (!file) return false;
    
    file.seekp(offset);
    file.write(buffer, size);
    return file.good();
}

std::string FileUtils::calculateMD5(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return "";
    
    MD5_CTX md5Context;
    MD5_Init(&md5Context);
    
    char buffer[4096];
    while (file.good()) {
        file.read(buffer, sizeof(buffer));
        MD5_Update(&md5Context, buffer, file.gcount());
    }
    
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &md5Context);
    
    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}

bool FileUtils::verifyMD5(const std::string& filename, const std::string& expected_md5) {
    std::string actual_md5 = calculateMD5(filename);
    return actual_md5 == expected_md5;
}

void FileUtils::showProgress(uint64_t current, uint64_t total, const std::string& operation) {
    float percentage = (float)current / total * 100;
    std::cout << "\r" << operation << ": " << std::fixed << std::setprecision(2)
              << percentage << "% (" << formatSize(current) << "/" 
              << formatSize(total) << ")" << std::flush;
}

std::string FileUtils::formatSize(uint64_t size) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double value = size;
    
    while (value >= 1024 && unit < 4) {
        value /= 1024;
        unit++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << value << " " << units[unit];
    return ss.str();
}

std::string FileUtils::formatSpeed(uint64_t bytes, double seconds) {
    double speed = bytes / seconds;
    return formatSize(speed) + "/s";
}

// NetworkUtils 实现
bool NetworkUtils::sendPacket(SOCKET socket, const Packet& packet) {
    // 发送包头
    if (send(socket, (const char*)&packet.header, sizeof(PacketHeader), 0) != sizeof(PacketHeader)) {
        return false;
    }
    
    // 发送数据部分
    if (packet.header.data_size > 0) {
        if (send(socket, packet.data, packet.header.data_size, 0) != packet.header.data_size) {
            return false;
        }
    }
    
    return true;
}

bool NetworkUtils::receivePacket(SOCKET socket, Packet& packet) {
    // 接收包头
    if (recv(socket, (char*)&packet.header, sizeof(PacketHeader), 0) != sizeof(PacketHeader)) {
        return false;
    }
    
    // 接收数据部分
    if (packet.header.data_size > 0) {
        packet.data = new char[packet.header.data_size];
        if (recv(socket, packet.data, packet.header.data_size, 0) != packet.header.data_size) {
            delete[] packet.data;
            return false;
        }
    } else {
        packet.data = nullptr;
    }
    
    return true;
}

bool NetworkUtils::waitForAck(SOCKET socket, uint32_t expected_seq, int timeout_ms) {
    Packet packet;
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        if (receivePacket(socket, packet)) {
            if (packet.header.type == ACK && packet.header.seq_num == expected_seq) {
                delete[] packet.data;
                return true;
            }
            delete[] packet.data;
        }
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (elapsed.count() >= timeout_ms) {
            return false;
        }
    }
}

bool NetworkUtils::sendAck(SOCKET socket, uint32_t seq_num) {
    Packet packet;
    packet.header.version = PROTOCOL_VERSION;
    packet.header.type = ACK;
    packet.header.seq_num = seq_num;
    packet.header.data_size = 0;
    packet.data = nullptr;
    
    return sendPacket(socket, packet);
}

bool NetworkUtils::sendError(SOCKET socket, ErrorCode code, const std::string& message) {
    Packet packet;
    packet.header.version = PROTOCOL_VERSION;
    packet.header.type = ERROR;
    packet.header.seq_num = 0;
    packet.header.data_size = message.length();
    packet.data = new char[message.length()];
    memcpy(packet.data, message.c_str(), message.length());
    
    bool result = sendPacket(socket, packet);
    delete[] packet.data;
    return result;
} 