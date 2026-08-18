#include "sockutil.h"

int create_socket(int domain, int type, int protocol)
{
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int bind_socket(int domain,int sockfd,int port, const char* ip)
{
    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family=domain;
    addr.sin_port=htons(port);
    if (ip && *ip) {
        if (inet_pton(domain, ip, &addr.sin_addr) <= 0) {
            fprintf(stderr, "无效的 IP 地址: %s\n", ip);
            return -1;
        }
    } else {
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
    }
    int ret=bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));
    return ret;
}

void set_socket_nonblocking(int sockfd)
{
    int flags = fcntl(sockfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(sockfd,F_SETFL,flags);
}

int set_socket_reuseaddr(int sockfd)
{
    int opt=1;
    int ret;
    ret=setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return ret;
}

ssize_t Read(int fd,void* buf,size_t len)
{
    size_t n;
    do{
        n=read(fd, buf, len);
    }while(errno==EINTR);
    if(n<0){
        perror("read error");
        return n;
    }
    else if(n==0){
        return n;
    }
    else{
        return n;
    }
}

ssize_t Write(int fd,const void* buf, size_t len)
{
    size_t n;
    do{
        n=write(fd, buf, len);
    }while(n==EINTR);
    if(n<0){
        perror("write error");
    }
    else if(n==0){
        return n;
    }
    else{
        return n;
    }
}

int connect_socket(const char* ip, int port)
{
    int sockfd = create_socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("inet_pton error");
        close(sockfd);
        return -1;
    }
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect error");
        close(sockfd);
        return -1;
    }
    return sockfd;
}