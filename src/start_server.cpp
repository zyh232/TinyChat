#include"server.h"


int main(){
    memset(xevents,0x00,sizeof(xevents));
    struct epoll_event events[_MAX_EVENTS_+1];
    memset(events,0x00,sizeof(events));

    int lfd=tcp_listen_create(8888);
    if(lfd<0){
        return -1;
    }

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
            if(xev->events&events[i].events){
                xev->call_back(gepfd,xev->fd);
            }
        }
    }



    return 0;
}