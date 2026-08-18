#ifndef _SERVER_H
#define _SERVER_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<errno.h>
#include<cstring>
#include<string>
#include<vector>
#include<map>
#include<set>

#define _MAX_EVENTS_ 1024
//第1025个用于存储lfd
#define _BUF_SIZE_ 4096

struct xevent{
    int fd;
    int events;
    void (*call_back)(int,int);               // 回调：epfd, fd（平时是 readMessage，有待写数据时切到 writeMessage）
    std::string read_buf;   // 接收缓冲区
    std::string write_buf;  // 发送缓冲区
    std::string user_name;  // 用户昵称
    bool in_use;            // 该槽位是否被占用
};

// 全局事件表：前 _MAX_EVENTS_ 个给客户端连接，最后一个给监听 socket
extern xevent xevents[_MAX_EVENTS_+1];

// 群聊表：群名 -> 成员 fd 集合
extern std::map<std::string, std::set<int>> groups;

// 群聊邀请名单：群名 -> 被邀请的 fd 集合（受邀后才能 JOIN）
extern std::map<std::string, std::set<int>> invitedGroups;

int tcp_listen_create(int port=8888, const char* ip=nullptr);
void eventadd(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev);
void eventmod(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev);
void eventdel(int gepfd,int fd,xevent* xev);
void initAccept(int gepfd,int lfd);
void readMessage(int gepfd,int fd);
void writeMessage(int gepfd,int fd);
void closeClient(int gepfd,int fd);

// ---- 聊天协议处理 ----
void sendTo(int gepfd, int fd, const std::string& s);          // 给单个连接发一条消息
void broadcastGroup(int gepfd, const std::string& gname, const std::string& msg, int except_fd=-1);
void handleLine(int gepfd, int fd, const std::string& line);   // 处理一行客户端命令
std::vector<std::string> split(const std::string& s);          // 按空白切分

#endif //_SERVER_H