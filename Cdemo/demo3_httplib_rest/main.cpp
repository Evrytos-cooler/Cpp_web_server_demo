#include <iostream>
#include "httplib.h" // 引入cpp-httplib头文件
#include "json.hpp"  // 引入nlohmann/json头文件

using json = nlohmann::json; // 定义json类型别名，方便使用

int main()
{
    // 设置控制台编码为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    httplib::Server svr; // 创建RESTful服务器对象

    // 定义/hello接口，支持GET请求，带参数name
    svr.Get("/hello", [](const httplib::Request &req, httplib::Response &res)
            {
        // 判断是否有name参数，没有则用"匿名"
        std::string name = req.has_param("name") ? req.get_param_value("name") : "匿名";
        // 构造json对象
        json j = {
            {"msg", "你好，" + name},
            {"param", name}
        };
        // 设置返回内容为json
        res.set_content(j.dump(), "application/json; charset=utf-8"); });

    // 定义/query接口，支持GET请求，带参数name和age
    svr.Get("/query", [](const httplib::Request &req, httplib::Response &res)
            {
        std::string name = req.has_param("name") ? req.get_param_value("name") : "";
        std::string age = req.has_param("age") ? req.get_param_value("age") : "";
        // 构造json对象
        json j = {
            {"name", name},
            {"age", age},
            {"info", "参数已收到"}
        };
        // 设置返回内容为json
        res.set_content(j.dump(), "application/json; charset=utf-8"); });

    // 定义/user接口，支持POST请求，接收JSON数据
    svr.Post("/user", [](const httplib::Request &req, httplib::Response &res)
             {
        try {
            // 解析请求体中的JSON数据
            json request_json = json::parse(req.body);
            
            // 构造响应JSON
            json response_json = {
                {"status", "success"},
                {"message", "数据已接收"},
                {"received_data", request_json}
            };
            
            // 设置返回内容为json
            res.set_content(response_json.dump(), "application/json; charset=utf-8");
        } catch (const std::exception& e) {
            res.status = 400; // 设置HTTP错误状态码为400
        } });

    std::cout << "监听8081端口..." << std::endl;
    svr.listen("0.0.0.0", 8081); // 启动服务器，监听8081端口
    return 0;                    // 返回0表示成功
}
