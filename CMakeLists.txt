cmake_minimum_required(VERSION 3.16)
project(BankManage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

if (MSVC)
    add_compile_options(/utf-8)
endif ()

add_executable(BankManage
        console_main.cpp
        include/Account.h
        src/Account.cpp
        include/CurrentAccount.h
        src/CurrentAccount.cpp
        include/FixedAccount.h
        src/FixedAccount.cpp
        include/CreditAccount.h
        src/CreditAccount.cpp
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
