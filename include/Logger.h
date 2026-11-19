#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>

// 静态操作审计日志工具类：负责以统一时间戳格式向独立文件追加记录关键业务事件，供管理员审查追踪
class Logger {
private:
    static std::string path;   // 日志文件路径，默认为 "data/logs.txt"

public:
    // 追加一条带时间戳的操作日志，若父级目录不存在会自动创建
    static void log(const std::string &message);

    // 顺序读取全部历史日志行，返回字符串列表
    static std::vector<std::string> getAllLogs();

    // 清空现有日志文件内容（以截断模式重写）
    static void clearAllLogs();

    // 自定义日志输出路径（主要供独立测试或切换目录使用）
    static void setPath(const std::string &npath) {
        path = npath;
    }
};

#endif // LOGGER_H
