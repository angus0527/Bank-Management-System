//
// Created by huang on 2026/10/21.
//

#ifndef LOGGER_H
#define LOGGER_H
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <vector>
#include<iostream>
#include <filesystem>


class Logger {
private:
    static std::string path;

    static std::string getCurrentTime();
public:
    static void log(const std::string& message);

    static std::vector<std::string> getAllLogs();

    static void clearAllLogs();

    static void setPath(const std::string& npath) {
        path = npath;
    }
};


#endif //LOGGER_H
