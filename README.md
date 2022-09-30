# C++ Socket Demo

- (`tcp-game.cpp`) A multithreading TCP game server listening on port 4000.
- (`udp-echo.cpp`) A UDP echo server listening on port 4000.
- (`http.cpp`) A multithreading HTTP Server on port 3000, with 0 - 999ms random delay.
- Graceful shutdown for TCP and HTTP.

Other versions:

- [Makefile](https://github.com/leancloud/cpp-socket-demo) (current)
- [CMake](https://github.com/leancloud/cpp-socket-demo/tree/cmake)
- Bazel (current)

## Build and start server

```
$ bazel run //:all
[UDP] Socket is listening on 4000
[TCP] Socket is listening on 4000
[HTTP] Socket is listening on 3000
```

## TCP Game

Establish two connections to server.

connection #1:

```
$ nc 127.0.0.1 4000
>> 1 players online, last number is 1, you must send the next number but skip numbers which multiples of 3 or includes the digit 3.
2   (your input)
>> 2 players online, 127.0.0.1:59385 sent 2.
>> 127.0.0.1:59897 lost, 2 players online.
4   (your input)
>> 1 players online, 127.0.0.1:59385 sent 4.
```

connection #2:

```
$ nc 127.0.0.1 4000
>> 2 players online, last number is 1, you must send the next number but skip numbers which multiples of 3 or includes the digit 3.
>> 2 players online, 127.0.0.1:59385 sent 2.
3   (your input)
>> you are lost
(disconnected)
```

## UDP Echo

```
$ nc -u 127.0.0.1 4000
hello                   (your input)
hello                   (response)
```

## HTTP

```
$ curl http://127.0.0.1:3000
Hello
```

## Graceful shutdown

You can send `SIGTERM` or `SIGINT` (`ctrl-C`) to start graceful shutdown:

```
$ ./cpp-socket
[UDP] Socket is listening on 4000
[TCP] Socket is listening on 4000
[HTTP] Socket is listening on 3000
[TCP] A connection is accepted now.
[TCP] 127.0.0.1 connected...
(ctrl-C)
[SIGNAL] 2 workers are running, waiting for them finish ...
(wait for all connections disconnect)
[TCP] 127.0.0.1 disconnected.
(exited)
```

## Documentation

- [云引擎服务总览](https://docs.leancloud.cn/sdk/engine/overview)
- 云引擎 C++ 运行环境（WIP）
- 云引擎游戏后端开发指南（WIP）
- [命令行工具 CLI 使用指南](https://docs.leancloud.cn/sdk/engine/cli/)
