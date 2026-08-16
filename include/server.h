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

int tcp_listen_create(int port);

#endif //_SERVER_H