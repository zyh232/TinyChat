#ifndef _SOCKUTIL_H
#define _SOCKUTIL_H

#include"server.h"
int create_socket(int domain,int type,int protocol);
int bind_socket(int domain,int sockfd,int port=8888);
void set_socket_nonblocking(int sockfd);
int set_socket_reuseaddr(int sockfd);
int listen_socket(int sockfd,int backlog=128);

ssize_t Read(int fd,void* buf,size_t len);
ssize_t Write(int fd,const void* buf,size_t len);

#endif //_SOCKUTIL_H
