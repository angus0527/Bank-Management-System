//
// Created by huang on 2026/10/21.
//

#include "../include/Logger.h"

std::string Logger::path = "data/logs.txt";

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::system_clock::to_time_t(now);
    auto localTime = std::localtime(&now_ms);

    std::stringstream ss;
    ss << std::put_time(localTime, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void Logger::log(const std::string &message) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    auto os = std::ofstream(path, std::ios::app);
    os << "[" << getCurrentTime() << "] " << message << std::endl;
    os.close();
}

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

void Logger::clearAllLogs() {
    auto os = std::ofstream(path, std::ios::trunc);
    os.close();
}
