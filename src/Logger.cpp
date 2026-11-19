#include "../include/Logger.h"

#include <iomanip>

std::string Logger::path = "data/logs.txt";

// 调用 C++17 chrono 系统时钟并转换为本地日历时间后格式化输出
std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::system_clock::to_time_t(now);
    auto localTime = std::localtime(&now_ms);

    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 写入一条审计日志：先用 filesystem 补全父目录防止 ofstream 静默打开失败，再以追加模式写文件
void Logger::log(const std::string &message) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    auto os = std::ofstream(path, std::ios::app);
    os << "[" << getCurrentTime() << "] " << message << std::endl;
    os.close();
}

// 按行读出 logs.txt 中的全部有效日志内容
std::vector<std::string> Logger::getAllLogs() {
    auto infstream = std::ifstream(path);
    std::vector<std::string> logs;
    std::string line;
    while (std::getline(infstream, line)) {
        if (!line.empty()) {
            logs.push_back(line);
        }
    }
    return logs;
}

// 清空日志文件
void Logger::clearAllLogs() {
    auto os = std::ofstream(path, std::ios::trunc);
    os.close();
}
