// 简易聊天室客户端
#include"client.h"
#include<sstream>

std::string to_upper(const std::string& s)
{
    std::string r = s;
    for (auto& c : r) c = toupper((unsigned char)c);
    return r;
}

bool is_known_command(const std::string& c)
{
    static const char* cmds[] = {
        "NICK","LIST","MYGROUPS","CREATE","JOIN","LEAVE","INVITE","RENAME","MSG","HELP","QUIT","EXIT"
    };
    for (const char* k : cmds)
        if (c == k) return true;
    return false;
}

// 取出 "rest" 中的第一个词（也就是原始命令行的第 2 个词）
std::string second_word(const std::string& s)
{
    std::istringstream iss(s);
    std::string w;
    iss >> w;
    return w;
}

// 把服务器消息解析并格式化打印
void print_server_msg(const std::string& msg)
{
    // 私聊：FROM <sender> <body>  ->  <sender>: <body>
    if (msg.rfind("FROM ", 0) == 0) {
        std::istringstream iss(msg);
        std::string tag, sender, body;
        iss >> tag >> sender;
        std::getline(iss, body);
        if (!body.empty() && body[0] == ' ') body.erase(0, 1);
        printf(C_CYAN "%s: %s" C_RESET "\n", sender.c_str(), body.c_str());
    }
    // 群聊：GROUP <group> <sender> <body>  ->  [group] <sender>: <body>
    else if (msg.rfind("GROUP ", 0) == 0) {
        std::istringstream iss(msg);
        std::string tag, group, sender, body;
        iss >> tag >> group >> sender;
        std::getline(iss, body);
        if (!body.empty() && body[0] == ' ') body.erase(0, 1);
        printf(C_YELLOW "[%s] %s: %s" C_RESET "\n", group.c_str(), sender.c_str(), body.c_str());
    }
    else if (msg.rfind("ERR ", 0) == 0)
        printf(C_RED "%s" C_RESET "\n", msg.c_str());
    else if (msg.rfind("SYS ", 0) == 0)
        printf(C_GREEN "%s" C_RESET "\n", msg.c_str());
    else
        printf("%s\n", msg.c_str());
    fflush(stdout);
}

void print_help()
{
    printf(C_GREEN
        "可用命令（大小写均可）：\n"
        "  nick <名字>                设置/修改昵称\n"
        "  list                       查看在线用户和群聊\n"
        "  mygroups                   查看你已加入的群聊\n"
        "  create <群名>              创建群聊（你是群主）\n"
        "  join <群名>                加入你已被邀请的群聊\n"
        "  leave <群名>               退出群聊\n"
        "  invite <用户> <群名>       邀请用户加入群聊（仅群成员可邀请）\n"
        "  rename <旧群名> <新群名>   修改群聊名称\n"
        "  msg <用户或群名> <内容>    给某用户私聊，或往某群聊发消息\n"
        "  help                       显示帮助\n"
        "  quit                       退出\n" C_RESET);
    fflush(stdout);
}

