# 实验一 

这是一个基于原生Socket实现的简单Web服务器示例。

## 环境要求

- C++编译器
- Make工具
- Linux/Unix操作系统（因为使用了POSIX socket API）

## 编译方法

在项目目录下执行：

```bash
make
```

## 运行方法

编译完成后，直接运行生成的可执行文件：

```bash
./webserver
```

服务器默认会在8080端口启动。可以通过浏览器访问 `http://localhost:8080` 来测试服务器。

## 功能特点

- 支持基本的HTTP GET请求
- 可以处理静态文件请求
- 简单的错误处理机制

## 注意事项

- 确保8080端口未被其他程序占用