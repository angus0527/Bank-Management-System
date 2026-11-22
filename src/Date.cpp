#include "../include/Date.h"

#include <ctime>
#include <iostream>
#include <sstream>

//5->05
static std::string twoDigits(int v) {
    return (v < 10 ? "0" : "") + std::to_string(v);
}

// 判断年份 y 是否为闰年
bool Date::isLeap(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

// 返回指定年月的天数
int Date::daysInMonth(int y, int m) {
    static const int table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) {
        return 0;
    }
    if (m == 2 && isLeap(y)) {
        return 29;
    }
    return table[m - 1];
}


Date::Date(int y, int m, int d) : year(y), month(m), day(d) {
    int maxDay = daysInMonth(y, m);
    if (y < 1900 || y > 9999 || maxDay == 0 || d < 1 || d > maxDay) {
        std::cout << "提示：日期不合法（" << y << "-" << m << "-" << d << "），已重置为 1900-01-01。\n";
        year = 1900;
        month = 1;
        day = 1;
    }
}

// 统计从 1900-01-01 到当前日期经过的天数，供 daysBetween 计算天数差
long long Date::toSerial() const {
    long long total = 0;
    for (int y = 1900; y < year; y++) {
        total += isLeap(y) ? 366 : 365;
    }
    for (int m = 1; m < month; m++) {
        total += daysInMonth(year, m);
    }
    total += day - 1;
    return total;
}


Date Date::addDays(long long n) const {
    int y = year;
    int m = month;
    int d = day;
    while (n > 0) {
        int dim = daysInMonth(y, m);
        //当前日期加上n后小于当前月的天数
        if (d + n <= dim) {
            d += static_cast<int>(n);
            n = 0;
        } else {
            n -= (dim - d + 1);
            d = 1;
            m++;
            if (m > 12) {
                m = 1;
                y++;
            }
        }
    }
    while (n < 0) {
        //说明减去n后还在当前月份
        if (d + n >= 1) {
            d += static_cast<int>(n);
            n = 0;
        } else {
            n += d;
            m--;
            if (m < 1) {
                m = 12;
                y--;
            }
            d = daysInMonth(y, m);
            //退回上一个月最后一天
        }
    }
    return Date(y, m, d);
}

//n可以是负数
Date Date::addMonths(int n) const {
    int y = year;
    int m = month + n;
    while (m > 12) {
        m -= 12;
        y++;
    }
    while (m < 1) {
        m += 12;
        y--;
    }
    int maxDay = daysInMonth(y, m);
    int d = (day > maxDay) ? maxDay : day;
    return Date(y, m, d);
}

// 获取当前系统日期
Date Date::today() {
    std::time_t t = std::time(nullptr);
    std::tm *local = std::localtime(&t);
    return Date(local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
}

// 获取当前日期时间字符串
std::string Date::nowDateTime() {
    std::time_t t = std::time(nullptr);
    std::tm *local = std::localtime(&t);
    return std::to_string(local->tm_year + 1900) + "-" +
           twoDigits(local->tm_mon + 1) + "-" +
           twoDigits(local->tm_mday) + " " +
           twoDigits(local->tm_hour) + ":" +
           twoDigits(local->tm_min) + ":" +
           twoDigits(local->tm_sec);
}

// 解析 "YYYY-MM-DD" 字符串为日期对象
Date Date::fromString(const std::string &s) {
    int y, m, d;
    char sep1, sep2;
    std::istringstream iss(s);
    if ((iss >> y >> sep1 >> m >> sep2 >> d) && sep1 == '-' && sep2 == '-') {
        return Date(y, m, d);
    }
    std::cout << "提示：日期格式应为 YYYY-MM-DD（收到 \"" << s << "\"），已重置为 1900-01-01。\n";
    return Date(1900, 1, 1);
}

// 将日期格式化为 "YYYY-MM-DD" 字符串
std::string Date::toString() const {
    return std::to_string(year) + "-" + twoDigits(month) + "-" + twoDigits(day);
}

// 计算从当前日期到 other 相隔多少天（other 更早时返回负数）
long long Date::daysBetween(const Date &other) const {
    return other.toSerial() - toSerial();
}
