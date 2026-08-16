#include"server.h"
#include"sockutil.h"

int tcp_listen_create(int port){
    int lfd;
    int ret;
    lfd=create_socket(AF_INET,SOCK_STREAM,0);
    ret=set_socket_reuseaddr(lfd);
    if(ret<0){
        perror("setsockopt error");
        return ret;
    }

    ret=bind_socket(AF_INET,lfd,port);
    if(ret<0){
        perror("bind error");
        return ret;
    }

    ret=listen(lfd, 128);
    if(ret<0){
        perror("listen error");
        return ret;
    }

    return ret;
}

void eventadd(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev)
{
    xev->fd=fd;
    xev->events=events;
    xev->call_back=call_back;
    
    struct epoll_event ev;
    memset(&ev,0x00,sizeof(ev));
    ev.data.ptr=xev;
    ev.events=events;

    epoll_ctl(gepfd, EPOLL_CTL_ADD, fd, &ev);
    return;
}

void initAccept(int gepfd,int lfd)
{
    int i;
    for(i=0;i<_MAX_EVENTS_;++i){
        if(xevents[i].fd==0){
            break;
        }
    }
    if(i==_MAX_EVENTS_){
        printf("too many clients\n");
        return;
    }
    int cfd;

    do{
        cfd=accept(cfd, NULL, NULL);
    }while(errno=EINTR);

    if(cfd<0){
        perror("accept error");
        return;
    }
   

}

void eventmod(int gepfd, int fd,int events,void(*call_back)(int,int),xevent* xev)
{
    xev->call_back=call_back;
    xev->events=events;
    xev->fd=fd;
    
    struct epoll_event ev;
    ev.data.ptr=xev;
    ev.events=events;

    epoll_ctl(gepfd, EPOLL_CTL_MOD, fd, &ev);

}
void eventdel(int gepfd,int fd,xevent* xev)
{
    memset(xev,0x00,sizeof(*xev));

    epoll_ctl(gepfd,EPOLL_CTL_DEL, fd, NULL);
}