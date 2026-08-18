#include"server.h"
#include"sockutil.h"
#include<sstream>
#include<cctype>

xevent xevents[_MAX_EVENTS_+1];
std::map<std::string, std::set<int>> groups;
std::map<std::string, std::set<int>> invitedGroups;

static const char* helpText =
    "可用命令（不区分大小写）：\n"
    "  NICK <名字>               设置/修改昵称\n"
    "  LIST                      查看在线用户和群聊\n"
    "  MYGROUPS                  查看你已加入的群聊\n"
    "  CREATE <群名>             创建群聊\n"
    "  JOIN <群名>               加入你已被邀请的群聊\n"
    "  LEAVE <群名>              退出群聊\n"
    "  INVITE <用户> <群名>      邀请某个用户加入群聊\n"
    "  RENAME <旧群名> <新群名>  修改群聊名称\n"
    "  MSG <用户或群名> <内容>   给某用户私聊，或往某群聊发消息\n"
    "  HELP                      显示帮助\n"
    "  QUIT                      退出";

// 根据 fd 找到对应的 xevent 下标，找不到返回 -1
static int find_index(int fd)
{
    for (int i = 0; i < _MAX_EVENTS_; ++i) {
        if (xevents[i].in_use && xevents[i].fd == fd)
            return i;
    }
    return -1;
}

static std::string to_upper(std::string s)
{
    for (auto& c : s) c = toupper((unsigned char)c);
    return s;
}

// 取命令第 2 个参数之后的内容作为消息正文
static std::string body_after(const std::string& line, const std::string& tok0, const std::string& tok1)
{
    size_t p = line.find(tok0);
    p = line.find(tok1, p);
    if (p == std::string::npos) return "";
    std::string body = line.substr(p + tok1.size());
    size_t q = body.find_first_not_of(" \t");
    return q == std::string::npos ? "" : body.substr(q);
}

std::vector<std::string> split(const std::string& s)
{
    std::vector<std::string> v;
    std::istringstream iss(s);
    std::string t;
    while (iss >> t) v.push_back(t);
    return v;
}

int tcp_listen_create(int port, const char* ip)
{
    int lfd = create_socket(AF_INET, SOCK_STREAM, 0);
    if (set_socket_reuseaddr(lfd) < 0) {
        perror("setsockopt error");
        return -1;
    }
    if (bind_socket(AF_INET, lfd, port, ip) < 0) {
        perror("bind error");
        return -1;
    }
    if (listen(lfd, 128) < 0) {
        perror("listen error");
        return -1;
    }
    set_socket_nonblocking(lfd);
    return lfd;
}

void eventadd(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev)
{
    xev->fd=fd;
    xev->events=events;
    xev->call_back=call_back;
    xev->in_use=true;

    struct epoll_event ev;
    memset(&ev,0x00,sizeof(ev));
    ev.data.ptr=xev;
    ev.events=events;

    epoll_ctl(gepfd, EPOLL_CTL_ADD, fd, &ev);
}

void eventmod(int gepfd,int fd,int events,void(*call_back)(int,int),xevent* xev)
{
    xev->call_back=call_back;
    xev->events=events;

    struct epoll_event ev;
    memset(&ev,0x00,sizeof(ev));
    ev.data.ptr=xev;
    ev.events=events;

    epoll_ctl(gepfd, EPOLL_CTL_MOD, fd, &ev);
}

void eventdel(int gepfd,int fd,xevent* xev)
{
    xev->fd = 0;
    xev->events = 0;
    xev->call_back = NULL;
    xev->read_buf.clear();
    xev->write_buf.clear();
    xev->user_name.clear();
    xev->in_use = false;
    epoll_ctl(gepfd,EPOLL_CTL_DEL, fd, NULL);
}

// 接受所有等待中的新连接
void initAccept(int gepfd,int lfd)
{
    int cfd;
    while (1) {
        cfd = accept(lfd, NULL, NULL);
        if (cfd > 0) {
            int i;
            for (i = 0; i < _MAX_EVENTS_; ++i)
                if (!xevents[i].in_use) break;

            if (i == _MAX_EVENTS_) {
                printf("too many clients, reject %d\n", cfd);
                close(cfd);
                return;
            }
            set_socket_nonblocking(cfd);
            xevent* xev = &xevents[i];
            xev->read_buf.clear();
            xev->write_buf.clear();
            xev->user_name.clear();
            eventadd(gepfd, cfd, EPOLLIN, readMessage, xev);
            sendTo(gepfd, cfd, "SYS 欢迎来到 TinyChat!请先设置昵称:NICK <你的名字>");
            printf("client %d connected\n", cfd);
        } else if (cfd < 0 && errno == EINTR) {
            continue;
        } else {
            break;   // EAGAIN：没有更多连接了
        }
    }
}

// 给单个连接发一条消息；一次写不完就缓存并注册 EPOLLOUT
void sendTo(int gepfd, int fd, const std::string& s)
{
    int i = find_index(fd);
    if (i < 0) return;
    xevent* xev = &xevents[i];

    std::string msg = s + "\n";

    if (xev->write_buf.empty()) {
        ssize_t n = write(fd, msg.data(), msg.size());
        if (n == (ssize_t)msg.size()) return;   
        if (n > 0) {
            xev->write_buf.assign(msg.data() + n, msg.size() - n);//剩下的放写缓冲区
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            xev->write_buf = msg;
        } else {
            return;   
        }
        // 有剩余数据：把回调切到 writeMessage，并注册 EPOLLOUT
        eventmod(gepfd, fd, EPOLLIN | EPOLLOUT, writeMessage, xev);
    } else {
        xev->write_buf += msg;
    }
}

//写回调
void writeMessage(int gepfd,int fd)
{
    int i = find_index(fd);
    if (i < 0) return;
    xevent* xev = &xevents[i];

    if (xev->write_buf.empty()) {
        eventmod(gepfd, fd, EPOLLIN, readMessage, xev);
        return;
    }
    ssize_t n = write(fd, xev->write_buf.data(), xev->write_buf.size());
    if (n > 0) {
        xev->write_buf.erase(0, n);
        if (xev->write_buf.empty())
            eventmod(gepfd, fd, EPOLLIN, readMessage, xev);
    } else if (n < 0 && errno != EAGAIN && errno != EINTR) {
        closeClient(gepfd, fd);
    }
}

// 读回调
void readMessage(int gepfd,int fd)
{
    int i = find_index(fd);
    if (i < 0) { close(fd); return; }
    xevent* xev = &xevents[i];

    char buf[_BUF_SIZE_];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        xev->read_buf.append(buf, n);
    }
    if (n == 0) {
        closeClient(gepfd, fd);   // 对端关闭
        return;
    }
    if (n < 0 && errno != EAGAIN && errno != EINTR) {
        closeClient(gepfd, fd);   // 读出错
        return;
    }

    // 处理所有完整行
    size_t pos;
    while ((pos = xev->read_buf.find('\n')) != std::string::npos) {
        std::string line = xev->read_buf.substr(0, pos);
        xev->read_buf.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        handleLine(gepfd, fd, line);
        if (!xevents[i].in_use) return;   // 该连接已被关闭
    }
}

// 处理一行客户端命令
void handleLine(int gepfd,int fd,const std::string& line)
{
    int i = find_index(fd);
    if (i < 0) return;
    xevent* xev = &xevents[i];

    std::vector<std::string> tok = split(line);
    if (tok.empty()) return;
    std::string cmd = to_upper(tok[0]);

    // ---- 还没有昵称，只能设置昵称/帮助/退出 ----
    if (xev->user_name.empty()) {
        if (cmd == "NICK" && tok.size() >= 2) {
            for (int k = 0; k < _MAX_EVENTS_; ++k)
                if (xevents[k].in_use && xevents[k].user_name == tok[1]) {
                    sendTo(gepfd, fd, "ERR 昵称 '" + tok[1] + "' 已被占用");
                    return;
                }
            xev->user_name = tok[1];
            sendTo(gepfd, fd, "SYS 你好，" + tok[1] + "！输入 HELP 查看命令");
        } else if (cmd == "HELP") {
            sendTo(gepfd, fd, "SYS " + std::string(helpText));
        } else if (cmd == "QUIT" || cmd == "EXIT") {
            closeClient(gepfd, fd);
        } else {
            sendTo(gepfd, fd, "ERR 请先用 NICK <你的名字> 设置昵称");
        }
        return;
    }

    const std::string& me = xev->user_name;

    if (cmd == "HELP") {
        sendTo(gepfd, fd, "SYS " + std::string(helpText));
    }
    else if (cmd == "NICK") {
        if (tok.size() < 2) { sendTo(gepfd, fd, "ERR 用法:NICK <新名字>"); return; }
        for (int k = 0; k < _MAX_EVENTS_; ++k)
            if (xevents[k].in_use && xevents[k].user_name == tok[1]) {
                sendTo(gepfd, fd, "ERR 昵称 '" + tok[1] + "' 已被占用");
                return;
            }
        xev->user_name = tok[1];
        sendTo(gepfd, fd, "SYS 你已改名为 " + tok[1]);
    }
    else if (cmd == "LIST") {
        std::string s = "SYS 在线用户：";
        bool first = true;
        for (int k = 0; k < _MAX_EVENTS_; ++k)
            if (xevents[k].in_use && !xevents[k].user_name.empty()) {
                if (!first) s += ", ";
                s += xevents[k].user_name;
                first = false;
            }
        if (first) s += "（无）";
        sendTo(gepfd, fd, s);

        s = "SYS 群聊：";
        first = true;
        for (auto& g : groups) {
            if (!first) s += ", ";
            s += g.first + "(" + std::to_string(g.second.size()) + "人)";
            first = false;
        }
        if (first) s += "（无）";
        sendTo(gepfd, fd, s);
    }
    else if (cmd == "MYGROUPS") {
        std::string s = "SYS 你已加入的群聊：";
        bool first = true;
        for (auto& g : groups) {
            if (g.second.count(fd)) {
                if (!first) s += ", ";
                s += g.first + "(" + std::to_string(g.second.size()) + "人)";
                first = false;
            }
        }
        if (first) s += "（无）";
        sendTo(gepfd, fd, s);
    }
    else if (cmd == "CREATE") {
        if (tok.size() < 2) { sendTo(gepfd, fd, "ERR 用法:CREATE <群名>"); return; }
        std::string gname = tok[1];
        if (groups.count(gname)) { sendTo(gepfd, fd, "ERR 群聊 '" + gname + "' 已存在"); return; }
        groups[gname].insert(fd);
        sendTo(gepfd, fd, "SYS 你创建了群聊 '" + gname + "'");
    }
    else if (cmd == "JOIN") {
        if (tok.size() < 2) { sendTo(gepfd, fd, "ERR 用法:JOIN <群名>"); return; }
        std::string gname = tok[1];
        auto it = groups.find(gname);
        if (it == groups.end()) {
            sendTo(gepfd, fd, "ERR 群聊 '" + gname + "' 不存在，如需创建请用 CREATE");
            return;
        }
        if (it->second.count(fd)) { sendTo(gepfd, fd, "SYS 你已在群聊 '" + gname + "' 中"); return; }
        // 必须先收到过邀请才能加入
        auto inv = invitedGroups.find(gname);
        if (inv == invitedGroups.end() || !inv->second.count(fd)) {
            sendTo(gepfd, fd, "ERR 你还没有收到群聊 '" + gname + "' 的邀请，无法加入");
            return;
        }
        inv->second.erase(fd);
        if (inv->second.empty()) invitedGroups.erase(inv);
        it->second.insert(fd);
        sendTo(gepfd, fd, "SYS 你已加入群聊 '" + gname + "'");
        broadcastGroup(gepfd, gname, "SYS " + me + " 加入了群聊", fd);
    }
    else if (cmd == "LEAVE") {
        if (tok.size() < 2) { sendTo(gepfd, fd, "ERR 用法:LEAVE <群名>"); return; }
        std::string gname = tok[1];
        auto it = groups.find(gname);
        if (it == groups.end() || !it->second.count(fd)) {
            sendTo(gepfd, fd, "ERR 你不在群聊 '" + gname + "' 中");
            return;
        }
        it->second.erase(fd);
        if (it->second.empty()) {
            groups.erase(it);
            invitedGroups.erase(gname);   // 群没人了，清掉邀请名单
        } else {
            broadcastGroup(gepfd, gname, "SYS " + me + " 退出了群聊");
        }
        sendTo(gepfd, fd, "SYS 你已退出群聊 '" + gname + "'");
    }
    else if (cmd == "INVITE") {
        if (tok.size() < 3) { sendTo(gepfd, fd, "ERR 用法:INVITE <用户> <群名>"); return; }
        std::string uname = tok[1], gname = tok[2];
        auto it = groups.find(gname);
        if (it == groups.end() || !it->second.count(fd)) {
            sendTo(gepfd, fd, "ERR 你不在群聊 '" + gname + "' 中，无法邀请");
            return;
        }
        int tfd = -1;
        for (int k = 0; k < _MAX_EVENTS_; ++k)
            if (xevents[k].in_use && xevents[k].user_name == uname) { tfd = xevents[k].fd; break; }
        if (tfd < 0) { sendTo(gepfd, fd, "ERR 用户 '" + uname + "' 不在线"); return; }
        if (it->second.count(tfd)) { sendTo(gepfd, fd, "ERR '" + uname + "' 已经在群里了"); return; }
        invitedGroups[gname].insert(tfd);   // 只记入邀请名单，等对方 JOIN 加入
        sendTo(gepfd, fd, "SYS 你已邀请 " + uname + " 加入群聊 '" + gname + "'");
        sendTo(gepfd, tfd, "SYS 你被 " + me + " 邀请加入群聊 '" + gname + "'，输入 JOIN " + gname + " 加入");
    }
    else if (cmd == "RENAME") {
        if (tok.size() < 3) { sendTo(gepfd, fd, "ERR 用法:RENAME <旧群名> <新群名>"); return; }
        std::string gname = tok[1], newname = tok[2];
        auto it = groups.find(gname);
        if (it == groups.end() || !it->second.count(fd)) {
            sendTo(gepfd, fd, "ERR 你不在群聊 '" + gname + "' 中，无法改名");
            return;
        }
        if (groups.count(newname)) { sendTo(gepfd, fd, "ERR 群名 '" + newname + "' 已存在"); return; }
        std::set<int> members = it->second;
        groups.erase(it);
        groups[newname] = members;
        // 同步邀请名单的群名
        auto inv = invitedGroups.find(gname);
        if (inv != invitedGroups.end()) {
            std::set<int> invited = inv->second;
            invitedGroups.erase(inv);
            invitedGroups[newname] = invited;
        }
        // 广播改名通知：带上旧名和新名，所有成员的客户端据此同步"当前群"
        broadcastGroup(gepfd, newname, "RENAMED " + gname + " " + newname);
    }
    else if (cmd == "MSG") {
        if (tok.size() < 3) { sendTo(gepfd, fd, "ERR 用法:MSG <用户或群名> <内容>"); return; }
        std::string target = tok[1];
        std::string body = body_after(line, tok[0], tok[1]);
        if (body.empty()) { sendTo(gepfd, fd, "ERR 消息内容不能为空"); return; }

        // 先按在线用户找，找到就私聊
        int tfd = -1;
        for (int k = 0; k < _MAX_EVENTS_; ++k)
            if (xevents[k].in_use && xevents[k].user_name == target) { tfd = xevents[k].fd; break; }
        if (tfd >= 0) {
            sendTo(gepfd, tfd, "FROM " + me + " " + body);
            return;
        }
        // 再按群聊找，找到且自己已加入就发群聊
        auto it = groups.find(target);
        if (it != groups.end() && it->second.count(fd)) {
            broadcastGroup(gepfd, target, "GROUP " + target + " " + me + " " + body);
            return;
        }
        sendTo(gepfd, fd, "ERR 找不到在线用户或你未加入的群聊 '" + target + "'");
    }
    else if (cmd == "QUIT" || cmd == "EXIT") {
        closeClient(gepfd, fd);
    }
    else {
        sendTo(gepfd, fd, "ERR 未知命令 '" + tok[0] + "'，输入 HELP 查看命令");
    }
}

// 向某个群的所有成员广播（except_fd 默认 -1，即全员）
void broadcastGroup(int gepfd,const std::string& gname,const std::string& msg,int except_fd)
{
    auto it = groups.find(gname);
    if (it == groups.end()) return;
    for (int fd : it->second) {
        if (fd == except_fd) continue;
        sendTo(gepfd, fd, msg);
    }
}

// 关闭一个客户端：从所有群移除、通知群成员、从 epoll 删除
void closeClient(int gepfd,int fd)
{
    int i = find_index(fd);
    if (i < 0) { close(fd); return; }
    xevent* xev = &xevents[i];
    std::string name = xev->user_name;

    // 收集该用户所在的所有群
    std::set<std::string> joined;
    for (auto& g : groups)
        if (g.second.count(fd))
            joined.insert(g.first);

    if (!name.empty())
        for (const std::string& g : joined)
            broadcastGroup(gepfd, g, "SYS 用户 " + name + " 下线了");

    // 从所有群中移除并清理空群
    for (auto it = groups.begin(); it != groups.end();) {
        it->second.erase(fd);
        if (it->second.empty()) {
            invitedGroups.erase(it->first);
            it = groups.erase(it);
        } else ++it;
    }
    // 从所有邀请名单中移除该用户
    for (auto it = invitedGroups.begin(); it != invitedGroups.end();) {
        it->second.erase(fd);
        if (it->second.empty()) it = invitedGroups.erase(it);
        else ++it;
    }

    if (!name.empty())
        printf("client %d (%s) disconnected\n", fd, name.c_str());
    else
        printf("client %d disconnected\n", fd);

    eventdel(gepfd, fd, xev);
    close(fd);
}