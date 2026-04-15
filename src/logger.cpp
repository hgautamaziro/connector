#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
 
void log(LogLevel level, const std::string& msg)
{
    // 🔹 Get current time
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
 
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
 
    // 🔹 Convert level to string
    const char* levelStr = "";
    switch (level) {
        case LogLevel::DEBUG: levelStr = "DEBUG"; break;
        case LogLevel::INFO:  levelStr = "INFO";  break;
        case LogLevel::WARN:  levelStr = "WARN";  break;
        case LogLevel::ERR: levelStr = "ERROR"; break;
    }
 
    // 🔹 Get thread id
    auto threadId = std::this_thread::get_id();
    std::cerr << "["
              << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "]"
              << "[" << levelStr << "]"
              << "[TID:" << threadId << "] "
              << msg
              << std::endl;
}