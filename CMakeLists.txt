cmake_minimum_required(VERSION 3.10)
project(TinyChat CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories(include)

# 可执行文件统一输出到 bin 目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 公共代码：client 和 server 都会用到
set(COMMON_SOURCES
    src/sockutil.cpp
)

# server 可执行程序
add_executable(server
    src/start_server.cpp
    src/server.cpp
    ${COMMON_SOURCES}
)

# client 可执行程序
add_executable(client
    src/start_client.cpp
    src/client.cpp
    ${COMMON_SOURCES}
)
