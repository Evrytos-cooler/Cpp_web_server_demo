# C++ Demo 项目

本项目包含三个 C++示例，适合 C++初学者学习网络编程和 RESTful 服务。

## 目录结构

- demo1_socket_webserver —— 基于 socket 的简单 Web 服务器
- demo2_mq —— 消息中间件服务器与客户端
- demo3_httplib_rest —— 使用 cpp-httplib 的 RESTful 服务器

---

## 1. demo1_socket_webserver

### 功能

- 监听 8080 端口，接收浏览器请求并打印请求内容。
- 返回一段 HTML 页面。

### 编译与运行

```sh
cd demo1_socket_webserver
make
./webserver
```

浏览器访问：http://localhost:8080

---

## 2. demo2_mq

### 功能

- 服务器监听 9000 端口，支持多个客户端连接。
- 任一客户端发消息，服务器转发给其他客户端。

### 编译与运行

```sh
cd demo2_mq
make
# 启动服务器
./server
# 新开多个终端，分别运行客户端
./client
```

客户端输入消息后回车即可，输入 exit 退出。

---

## 3. demo3_httplib_rest

### 功能

- 使用[cpp-httplib](https://github.com/yhirose/cpp-httplib)和[nlohmann/json](https://github.com/nlohmann/json)实现 RESTful 服务。
- 支持两个 GET 接口：
  - `/hello?name=xxx` 返回带参数的 json
  - `/query?name=xxx&age=yyy` 返回参数 json

### 编译与运行

```sh
cd demo3_httplib_rest
make
./rest_server
```

浏览器访问：

- http://localhost:8081/hello?name=张三
- http://localhost:8081/query?name=李四&age=20

---

如有问题欢迎提 issue。
