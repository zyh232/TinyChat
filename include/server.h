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

#define _MAX_EVENTS_ 1024
//第1025个用于存储lfd
#define _BUF_SIZE_ 1024
struct xevent{
    int fd;
    int events;
    void (*call_back)(int,int);
    char buf[_BUF_SIZE_];
    size_t len;
    char user_name[_BUF_SIZE_];
}xevents[_MAX_EVENTS_+1];
int tcp_listen_create(int port=8888);
void eventadd(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev);
void eventmod(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev);
void eventdel(int gepfd,int fd,xevent* xev);
void initAccept(int gepfd,int lfd);
void readMessage();
void writeMessage();

#endif //_SERVER_H