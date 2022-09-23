# C++ Socket Demo

- A multithreading TCP broadcast server listening on port 4000.
- A UDP echo server listening on port 4000.
- A multithreading HTTP Server on port 3000, with 0 - 999ms random delay.
- Graceful shutdown for TCP and HTTP.

Other versions:

- Makefile (current)
- [CMake](https://github.com/leancloud/cpp-socket-demo/tree/cmake)
- [Bazel](https://github.com/leancloud/cpp-socket-demo/tree/bazel)

## Usages

Build and start server:

```
$ make && ./cpp-socket
```

To test TCP broadcast, you can open two connections to server:

```
$ nc 127.0.0.1 4000     (connection #1)
hello                   (your input)
hello                   (broadcast message)
$ nc 127.0.0.1 4000     (connection #2)
hello                   (broadcast message from #1)
```

Test UDP:

```
$ nc -u 127.0.0.1 4000
hello                   (your input)
hello                   (response)
```

Test HTTP:

```
$ curl http://127.0.0.1:3000
Hello
```

# Documentation

- [云引擎服务总览](https://docs.leancloud.cn/sdk/engine/overview)
- 云引擎 C++ 运行环境（WIP）
- 云引擎游戏后端开发指南（WIP）
- [命令行工具 CLI 使用指南](https://docs.leancloud.cn/sdk/engine/cli/)
