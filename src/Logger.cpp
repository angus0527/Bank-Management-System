#include "../include/Logger.h"

#include <filesystem>
#include <fstream>

#include "../include/Date.h"

std::string Logger::path = "data/logs.txt";

// 写入一条审计日志：先补全父目录防止写文件失败，再以追加模式写入时间戳和事件
void Logger::log(const std::string &message) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream out(path, std::ios::app);
    out << "[" << Date::nowDateTime() << "] " << message << "\n";
}

// 按行读出 logs.txt 中的全部有效日志内容
std::vector<std::string> Logger::getAllLogs() {
    std::ifstream in(path);
    std::vector<std::string> logs;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            logs.push_back(line);
        }
    }
    return logs;
}

// 清空日志文件
void Logger::clearAllLogs() {
    std::ofstream out(path, std::ios::trunc);
}
