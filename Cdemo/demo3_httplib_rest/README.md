# 实验3 REST API 

这是一个使用cpp-httplib库实现的REST API服务器示例。

## 环境要求

- C++编译器
- Make工具
- 依赖库：
  - cpp-httplib（已包含在项目中）
  - nlohmann/json（已包含在项目中）

## 编译方法

在项目目录下执行：

```bash
make
```

## 运行方法

编译完成后，运行生成的可执行文件：

```bash
./rest_server
```

服务器默认会在8080端口启动。

## API 接口

服务器提供以下REST API端点：

- GET `/api/hello` - 返回问候消息
- GET `/api/data` - 获取数据
- POST `/api/data` - 提交数据

## 测试API

可以使用curl命令测试API：

```bash
# 测试GET请求
curl http://localhost:8080/api/hello
curl http://localhost:8080/api/data

# 测试POST请求
curl -X POST -H "Content-Type: application/json" -d '{"key":"value"}' http://localhost:8080/api/data
```

## 功能特点

- 基于cpp-httplib实现HTTP服务器
- 支持RESTful API设计
- 使用JSON进行数据交换
- 支持GET和POST请求方法

## 注意事项

- 确保8080端口未被其他程序占用
- 需要网络连接权限