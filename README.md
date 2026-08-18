# TinyChat 简易聊天室

一个基于 Linux epoll 的简易多人聊天室，使用 C++11 编写。支持用户昵称、私聊、群聊、邀请进群、修改群名等功能。

## 功能特性

- **用户昵称**：连接后设置昵称，可随时修改，昵称全局唯一
- **私聊**：`msg <用户> <内容>` 一对一发送消息，只有对方能看到
- **群聊**：
  - `create <群名>` 创建群聊
  - `invite <用户> <群名>` 群成员邀请其他用户进群
  - `join <群名>` 加入**已被邀请**的群聊
  - `leave <群名>` 退出群聊
  - `rename <旧群名> <新群名>` 修改群名，全群成员自动同步
  - `msg <群名> <内容>` 往群聊发消息（加入群后直接输入文字也会发到当前群）
- **在线用户 / 群聊列表**：`list` 查看
- **高并发**：服务端采用 epoll 单线程事件驱动，非阻塞 IO

## 技术要点

- C++11，Linux socket 编程
- 服务端：epoll + 非阻塞 IO，事件驱动，支持大量并发连接
- 客户端：`select` 同时监听键盘输入和网络数据
- 协议：行式文本协议，`\n` 分隔消息，支持中文（按字节处理）
- 构建：CMake（>= 3.10）

## 构建

依赖：Linux + g++ + CMake

```bash
cmake -B build
cmake --build build
```

生成的可执行文件位于 `bin/` 目录：

```bash
./bin/server                        # 监听所有网卡的 8888 端口
./bin/server 127.0.0.1              # 只监听本机回环
./bin/server 192.168.1.10 9000      # 绑定指定地址和端口
./bin/client                        # 连接本机服务器
./bin/client 192.168.1.10           # 连接指定 IP 的服务器
./bin/client 192.168.1.10 9000      # 指定 IP 和端口
```

## 命令一览

| 命令 | 说明 |
| --- | --- |
| `nick <名字>` | 设置 / 修改昵称 |
| `create <群名>` | 创建群聊 |
| `join <群名>` | 加入已被邀请的群聊 |
| `leave <群名>` | 退出群聊 |
| `mygroups` | 查看你已加入的群聊 |
| `invite <用户> <群名>` | 邀请用户进群 |
| `rename <旧群名> <新群名>` | 修改群名 |
| `msg <用户或群名> <内容>` | 私聊某用户，或往某群聊发消息 |
| `list` | 查看在线用户和群聊 |
| `help` | 显示帮助 |
| `quit` | 退出 |

> 提示：命令不区分大小写；`msg` 会先按在线用户匹配，再按群聊匹配。

## 目录结构

```
TinyChat
├── bin
│   ├── client
│   └── server
├── build
│   ├── CMakeCache.txt
│   ├── CMakeFiles
│   ├── cmake_install.cmake
│   └── Makefile
├── CMakeLists.txt
├── include
│   ├── client.h        # 客户端函数与常量声明
│   ├── server.h        # 服务端数据结构与协议声明
│   └── sockutil.h      # socket 封装函数声明
├── README.md
└── src
    ├── client.cpp      # 客户端逻辑（消息解析与打印）
    ├── server.cpp      # 服务端核心逻辑（epoll + 协议处理）
    ├── sockutil.cpp    # socket 封装实现（创建/绑定/连接）
    ├── start_client.cpp # 客户端入口
    └── start_server.cpp # 服务端入口
```