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

int bind_socket(int domain,int sockfd,int port=8888)
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_family=domain;
    addr.sin_port=port;
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

ssize_t Read(int fd,void* buf,size_t len){
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
