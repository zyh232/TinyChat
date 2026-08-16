#include"sockutil.h"

int create_socket(int domain,int type,int protocol)
{
    int sockfd=socket(domain,type,protocol);
    if(sockfd<0){
        perror("socket error");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}   

int bind_socket(int sockfd,const struct sockaddr* addr,socklen_t len)
{

}