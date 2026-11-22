#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>


class Logger {
private:
    static std::string path;   // 日志文件路径

public:
    // 记一条日志
    static void log(const std::string &message);

    // 拿所有日志
    static std::vector<std::string> getAllLogs();

    // 清空日志
    static void clearAllLogs();

    // 自定义日志输出路径
    static void setPath(const std::string &npath) {
        path = npath;
    }
};

#endif // LOGGER_H
