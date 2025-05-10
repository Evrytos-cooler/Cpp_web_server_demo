cmake_minimum_required(VERSION 3.10)
project(file_transfer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找OpenSSL库
find_package(OpenSSL REQUIRED)

# 添加头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 添加源文件
set(SOURCES
    src/utils.cpp
    src/server.cpp
    src/client.cpp
)

# 创建可执行文件
add_executable(server src/server.cpp src/utils.cpp)
add_executable(client src/client.cpp src/utils.cpp)

# 链接OpenSSL库
target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(client PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# 设置编译选项
if(MSVC)
    target_compile_options(server PRIVATE /W4)
    target_compile_options(client PRIVATE /W4)
else()
    target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)
endif() 