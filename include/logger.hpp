#pragma once
#include <string>
 
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERR
};
 
void log(LogLevel level, const std::string& msg);
#define LOG_DEBUG(msg) log(LogLevel::DEBUG, std::string(__FUNCTION__) + " | " + msg)
#define LOG_INFO(msg)  log(LogLevel::INFO,  std::string(__FUNCTION__) + " | " + msg)
#define LOG_WARN(msg)  log(LogLevel::WARN,  std::string(__FUNCTION__) + " | " + msg)
#define LOG_ERROR(msg) log(LogLevel::ERR, std::string(__FUNCTION__) + " | " + msg)