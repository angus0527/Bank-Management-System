//
// Created by angus on 2026/10/29.
//

#include "../include/Date.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <stdexcept>

bool Date::isLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int Date::daysInMonth(int y, int m) {
    static const int table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) {
        throw std::invalid_argument("月份必须在 1~12 之间");
    }
    if (m == 2 && isLeap(y)) {
        return 29;
    }
    return table[m - 1];
}

Date::Date(int y, int m, int d) : year(y), month(m), day(d) {
    if (y < 1900 || y > 9999) {
        throw std::invalid_argument("年份超出合理范围 [1900, 9999]");
    }
    if (m < 1 || m > 12) {
        throw std::invalid_argument("月份必须在 1~12 之间");
    }
    if (d < 1 || d > daysInMonth(y, m)) {
        throw std::invalid_argument("日期与年月不匹配");
    }
}

long long Date::toSerial() const {
    // days_from_civil：把公历日期映射为距 1970-01-01 的天数
    long long y = year;
    unsigned m = static_cast<unsigned>(month);
    unsigned d = static_cast<unsigned>(day);
    if (m <= 2) {
        y -= 1;
    }
    long long era = (y >= 0 ? y : y - 399) / 400;
    unsigned yoe = static_cast<unsigned>(y - era * 400);                 // [0, 399]
    unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;       // [0, 365]
    unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;                // [0, 146096]
    return era * 146097 + static_cast<long long>(doe) - 719468;
}

Date Date::addDays(long long n) const {
    // civil_from_days：toSerial 的逆运算
    long long z = toSerial() + n;
    z += 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = static_cast<unsigned>(z - era * 146097);              // [0, 146096]
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    long long y = static_cast<long long>(yoe) + era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);              // [0, 365]
    unsigned mp = (5 * doy + 2) / 153;                                   // [0, 11]
    unsigned d = doy - (153 * mp + 2) / 5 + 1;                           // [1, 31]
    unsigned m = mp + (mp < 10 ? 3 : -9);                                // [1, 12]
    return Date(static_cast<int>(y + (m <= 2)), static_cast<int>(m), static_cast<int>(d));
}

Date Date::addMonths(int n) const {
    long long total = static_cast<long long>(month) - 1 + n;
    int y = year + static_cast<int>(total / 12);
    int m = static_cast<int>(total % 12);
    if (m < 0) { // C++ 整除向零取整，负数月份要修正
        m += 12;
        y -= 1;
    }
    m += 1;
    int d = day > daysInMonth(y, m) ? daysInMonth(y, m) : day; // 月末收敛
    return Date(y, m, d);
}

Date Date::today() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm *local = std::localtime(&t);
    return Date(local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
}

std::string Date::nowDateTime() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm *local = std::localtime(&t);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                  local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
                  local->tm_hour, local->tm_min, local->tm_sec);
    return buf;
}

Date Date::fromString(const std::string &s) {
    int y, m, d;
    char sep1, sep2;
    std::istringstream iss(s);
    if ((iss >> y >> sep1 >> m >> sep2 >> d) && sep1 == '-' && sep2 == '-') {
        return Date(y, m, d); // Date 构造函数负责合法性校验
    }
    throw std::invalid_argument("日期格式应为 YYYY-MM-DD: \"" + s + "\"");
}

std::string Date::toString() const {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", year, month, day);
    return buf;
}

long long Date::daysBetween(const Date &other) const {
    return other.toSerial() - toSerial();
}
