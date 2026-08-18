#include"server.h"


int main(int argc,char* argv[]){
    const char* ip = argc > 1 ? argv[1] : nullptr;   // 默认 0.0.0.0，监听所有网卡
    int port = argc > 2 ? atoi(argv[2]) : 8888;

    struct epoll_event events[_MAX_EVENTS_+1];
    memset(events,0x00,sizeof(events));

    int lfd=tcp_listen_create(port, ip);
    if(lfd<0){
        return -1;
    }
    printf("TinyChat server listening on %s:%d\n", ip ? ip : "0.0.0.0", port);

    int gepfd=epoll_create(_MAX_EVENTS_+1);
    if(gepfd==-1){
        perror("epoll_create error");
        return -1;
    }
    eventadd(gepfd, lfd, EPOLLIN, initAccept, &xevents[_MAX_EVENTS_]);

    int nready;
    xevent* xev;
    while(1){
        nready=epoll_wait(gepfd, events, _MAX_EVENTS_+1,-1);
        if(nready<0){
            if(errno==EINTR){
                continue;
            }
            perror("epoll_wait error");
            break;
        }

        for(int i=0;i<nready;++i){
            xev=(xevent*)events[i].data.ptr;
            xev->call_back(gepfd, xev->fd);
        }
    }

    return 0;
}