#include "../include/Logger.h"

#include <filesystem>
#include <fstream>

#include "../include/Date.h"

std::string Logger::path = "data/logs.txt";

// 记一条日志
void Logger::log(const std::string &message) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    std::ofstream out(path, std::ios::app);
    out << "[" << Date::nowDateTime() << "] " << message << "\n";
}

// 拿所有日志
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

// 清空日志
void Logger::clearAllLogs() {
    std::ofstream out(path, std::ios::trunc);
}
