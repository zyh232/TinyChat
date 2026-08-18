#include"client.h"
#include<sstream>

int main(int argc, char* argv[])
{
    const char* ip = argc > 1 ? argv[1] : "127.0.0.1";
    int port = argc > 2 ? atoi(argv[2]) : 8888;

    int sockfd = connect_socket(ip, port);
    if (sockfd < 0) return -1;

    std::string recv_buf;
    std::string current_group;

    printf(C_GREEN "已连接服务器 %s:%d\n" C_RESET, ip, port);
    printf(C_GREEN "请先设置昵称:nick <你的名字>\n" C_RESET);
    print_help();

    char line[BUF_SIZE];
    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(sockfd, &rfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        int ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("select error");
            break;
        }

        // 服务器发来数据
        if (FD_ISSET(sockfd, &rfds)) {
            char buf[BUF_SIZE];
            ssize_t n = read(sockfd, buf, sizeof(buf));
            if (n <= 0) {
                printf(C_RED "与服务器断开连接\n" C_RESET);
                break;
            }
            recv_buf.append(buf, n);
            size_t pos;
            while ((pos = recv_buf.find('\n')) != std::string::npos) {
                std::string msg = recv_buf.substr(0, pos);
                recv_buf.erase(0, pos + 1);
                if (!msg.empty() && msg.back() == '\r') msg.pop_back();
                if (!msg.empty()) {
                    // 群改名通知
                    if (msg.rfind("RENAMED ", 0) == 0) {
                        std::istringstream iss(msg);
                        std::string tag, oldg, newg;
                        iss >> tag >> oldg >> newg;
                        if (current_group == oldg) current_group = newg;
                        printf(C_GREEN "群聊 '%s' 已改名为 '%s'" C_RESET "\n",
                               oldg.c_str(), newg.c_str());
                        fflush(stdout);
                    } else {
                        print_server_msg(msg);
                    }
                }
            }
        }

        // 键盘输入
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            if (!fgets(line, sizeof(line), stdin)) break;
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
                line[--len] = 0;
            if (len == 0) continue;

            std::string input(line, len);
            std::istringstream iss(input);
            std::string first;
            iss >> first; //取命令
            std::string rest;
            std::getline(iss, rest);   
            std::string cmd = to_upper(first);

            bool quit = false;
            std::string to_send;

            if (cmd == "HELP") {
                print_help();
                continue;
            } else if (cmd == "QUIT" || cmd == "EXIT") {
                quit = true;
            } else if (cmd == "JOIN" || cmd == "CREATE") {
                std::string gname = second_word(rest);
                if (!gname.empty()) current_group = gname;
                to_send = input;
            } else if (cmd == "LEAVE") {
                std::string gname = second_word(rest);
                if (!gname.empty() && current_group == gname) current_group.clear();
                to_send = input;
            } else if (cmd == "RENAME") {
                std::string oldg = second_word(rest);
                if (!oldg.empty() && current_group == oldg) {
                    std::string s2 = rest;
                    size_t p = s2.find(oldg);
                    if (p != std::string::npos) s2 = s2.substr(p + oldg.size());
                    std::string newg = second_word(s2);
                    if (!newg.empty()) current_group = newg;
                }
                to_send = input;
            } else if (cmd == "GMSG") {
                printf(C_YELLOW "gmsg 命令已移除，群聊消息请用 msg <群名> <内容> 发送\n" C_RESET);
                continue;
            } else if (is_known_command(cmd)) {
                to_send = input;
            } else {
                if (current_group.empty()) {
                    printf(C_YELLOW "你不在任何群聊中，私聊请用 msg <用户> <内容>，或先 join <群名> 加入群聊\n" C_RESET);
                    continue;
                }
                to_send = "MSG " + current_group + " " + input;
            }

            if (quit) {
                write(sockfd, "QUIT\n", 5);
                break;
            }

            std::string wire = to_send + "\n";
            if (write(sockfd, wire.data(), wire.size()) < 0) {
                perror("write error");
                break;
            }
        }
    }

    close(sockfd);
    printf("已退出聊天室\n");
    return 0;
}