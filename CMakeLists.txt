cmake_minimum_required(VERSION 3.16)
project(BankManage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 将 include 目录加入头文件搜索路径
include_directories(include)

# MSVC 按系统代码页解析含中文的源文件，需显式指定 UTF-8（MinGW 默认即为 UTF-8）
if (MSVC)
    add_compile_options(/utf-8)
endif ()

add_executable(BankManage
        console_main.cpp
        include/Account.h
        src/Account.cpp
        include/CurrentAccount.h
        src/CurrentAccount.cpp
        include/User.h
        src/User.cpp
        include/Transaction.h
        src/Transaction.cpp
        include/Bank.h
        src/Bank.cpp
        include/Date.h
        src/Date.cpp
        include/Storage.h
        src/Storage.cpp
        include/Logger.h
        src/Logger.cpp
)
