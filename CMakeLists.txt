cmake_minimum_required(VERSION 3.16)
project(BankManage)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.12.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Widgets)

include_directories(include)

if (MSVC)
    add_compile_options(/utf-8)
endif ()

add_executable(BankManage
        gui_main.cpp
        include/LoginDialog.h
        src/LoginDialog.cpp
        include/DepositWithdrawDialog.h
        src/DepositWithdrawDialog.cpp
        include/TransferDialog.h
        src/TransferDialog.cpp
        include/OpenAccountDialog.h
        src/OpenAccountDialog.cpp
        include/MainWindow.h
        src/MainWindow.cpp
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

target_link_libraries(BankManage PRIVATE Qt6::Widgets)

if (WIN32)
    add_custom_command(TARGET BankManage POST_BUILD
        COMMAND "${CMAKE_PREFIX_PATH}/bin/windeployqt.exe" --no-translations "$<TARGET_FILE:BankManage>"
        COMMENT "Auto-deploying Qt dependencies..."
    )
endif ()

add_executable(BankConsole
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


