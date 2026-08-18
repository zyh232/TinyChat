#ifndef _CLIENT_H
#define _CLIENT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <cctype>
#include <string>
#include <sys/select.h>
#include "sockutil.h"

#define BUF_SIZE 4096

// ANSI 颜色
#define C_RESET  "\033[0m"
#define C_GREEN  "\033[32m"
#define C_YELLOW "\033[33m"
#define C_CYAN   "\033[36m"
#define C_RED    "\033[31m"

std::string to_upper(const std::string& s);
bool is_known_command(const std::string& c);
std::string second_word(const std::string& s);
void print_server_msg(const std::string& msg);
void print_help();

#endif //_CLIENT_H