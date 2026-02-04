#ifndef GB28181_LOGGER_H
#define GB28181_LOGGER_H

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace gb28181 {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void SetLevel(LogLevel level) {
        level_ = level;
    }

    void SetLogFile(const std::string& filename) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.close();
        }
        file_.open(filename, std::ios::out | std::ios::app);
    }

    void Log(LogLevel level, const std::string& message) {
        if (level < level_) {
            return;
        }

        std::string log = FormatLog(level, message);

        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << log << std::endl;

        if (file_.is_open()) {
            file_ << log << std::endl;
            file_.flush();
        }
    }

private:
    Logger() : level_(LogLevel::INFO) {}

    std::string FormatLog(LogLevel level, const std::string& message) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        ss << " [" << LevelToString(level) << "] ";
        ss << message;

        return ss.str();
    }

    std::string LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG:   return "DEBUG";
            case LogLevel::INFO:    return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR:   return "ERROR";
            case LogLevel::FATAL:   return "FATAL";
            default:                return "UNKNOWN";
        }
    }

private:
    LogLevel level_;
    std::ofstream file_;
    std::mutex mutex_;
};

#define LOG_DEBUG(msg)  Logger::Instance().Log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg)   Logger::Instance().Log(LogLevel::INFO, msg)
#define LOG_WARN(msg)   Logger::Instance().Log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg)  Logger::Instance().Log(LogLevel::ERROR, msg)
#define LOG_FATAL(msg)  Logger::Instance().Log(LogLevel::FATAL, msg)

} // namespace gb28181

#endif // GB28181_LOGGER_H
