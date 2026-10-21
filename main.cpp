#include "Logger.h"
#include <windows.h>

int main() {
    SetConsoleOutputCP(CP_UTF8);
    Logger::clearAllLogs();
    Logger::log("系统管理员启动了系统");
    Logger::log("张三(20001) 登录成功");
    for (const auto &logLine: Logger::getAllLogs()) {
        std::cout << logLine << std::endl;
    }
    return 0;
}
