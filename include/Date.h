//
// Created by angus on 2026/10/29.
//

#ifndef DATE_H
#define DATE_H

#include <string>

// 日期工具类（README §2 设计约定 2：日期自己实现，不用第三方库）
// 内部用"公历天数序号"（距 1970-01-01 的天数）实现加减与天数差，报告可写的设计点
class Date {
    int year;
    int month;
    int day;

public:
    // 非法日期（如 2026-02-30）抛 std::invalid_argument
    Date(int y, int m, int d);

    // 当前系统日期
    static Date today();

    // 解析 "2026-10-29"，格式不符或日期非法抛 std::invalid_argument
    static Date fromString(const std::string &s);

    // 当前日期时间字符串 "2026-10-29 19:00:05"（交易流水时间戳用）
    static std::string nowDateTime();

    static bool isLeap(int y);

    static int daysInMonth(int y, int m);

    // 距 1970-01-01 的天数（proleptic Gregorian，霍华德·辛纳特算法）
    long long toSerial() const;

    // 从 this 到 other 相隔几天（other 早于 this 为负数）
    long long daysBetween(const Date &other) const;

    Date addDays(long long n) const;

    // 加 n 个月，月末自动收敛：2026-01-31 加 1 个月 → 2026-02-28（定期到期日用）
    Date addMonths(int n) const;

    // 格式化为 "2026-10-29"
    std::string toString() const;

    int getYear() const { return year; }

    int getMonth() const { return month; }

    int getDay() const { return day; }

    bool operator==(const Date &o) const { return year == o.year && month == o.month && day == o.day; }

    bool operator!=(const Date &o) const { return !(*this == o); }

    bool operator<(const Date &o) const { return toSerial() < o.toSerial(); }

    bool operator<=(const Date &o) const { return toSerial() <= o.toSerial(); }

    bool operator>(const Date &o) const { return o < *this; }

    bool operator>=(const Date &o) const { return o <= *this; }
};

#endif //DATE_H
