cmake_minimum_required(VERSION 3.31)
project(BankManage)

set(CMAKE_CXX_STANDARD 26)

# 将 include 目录加入头文件搜索路径
include_directories(include)

add_executable(BankManage main.cpp
        include/Account.h
        src/Account.cpp
        src/User.cpp
        include/User.h
        src/Admin.cpp
)
