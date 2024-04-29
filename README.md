# constellation

## 说明
在[测试文件](./tests/test_add_exit.cc)中添加了启动controller的代码，目前controller是一个空的控制器，对于接收到的请求无任何响应。但是能够解决在`Van`层调用`SendSignaltoController`时找不到上层的问题。

目前本仓库完成：
- 基本的项目工程结构