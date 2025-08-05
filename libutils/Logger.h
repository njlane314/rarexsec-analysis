#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <cstdlib>

namespace analysis {

enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLevel(LogLevel level) {
        level_ = level;
    }

    template<typename... Args>
    void debug(const std::string& context, const Args&... args) {
        log(LogLevel::DEBUG, context, args...);
    }

    template<typename... Args>
    void info(const std::string& context, const Args&... args) {
        log(LogLevel::INFO, context, args...);
    }

    template<typename... Args>
    void warn(const std::string& context, const Args&... args) {
        log(LogLevel::WARN, context, args...);
    }

    template<typename... Args>
    void error(const std::string& context, const Args&... args) {
        log(LogLevel::ERROR, context, args...);
    }

    template<typename... Args>
    void fatal(const std::string& context, const Args&... args) {
        log(LogLevel::FATAL, context, args...);
        std::exit(EXIT_FAILURE);
    }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    template<typename T, typename... Args>
    void printArgs(std::ostream& os, const T& first, const Args&... rest) const {
        os << first;
        if constexpr (sizeof...(rest) > 0) {
            os << " ";
            printArgs(os, rest...);
        }
    }

    void printArgs(std::ostream&) const {}

    template<typename... Args>
    void log(LogLevel level,
             const std::string& context,
             const Args&... args) {
        if (level >= level_) {
            std::lock_guard<std::mutex> lock(mutex_);
            auto now = std::chrono::system_clock::now();
            auto t   = std::chrono::system_clock::to_time_t(now);
            std::cout << "[" << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S") << "] "
                      << "[" << levelToString(level) << "] "
                      << "[" << context << "] ";
            printArgs(std::cout, args...);
            std::cout << std::endl;
        }
    }

    const char* levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO:  return "INFO";
            case LogLevel::WARN:  return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
        }
        return "UNKNOWN";
    }

    LogLevel        level_ = LogLevel::INFO;
    mutable std::mutex mutex_;
};

namespace log {
    template<typename... Args>
    inline void debug(const std::string& ctx, const Args&... args) {
        Logger::getInstance().debug(ctx, args...);
    }

    template<typename... Args>
    inline void info(const std::string& ctx, const Args&... args) {
        Logger::getInstance().info(ctx, args...);
    }

    template<typename... Args>
    inline void warn(const std::string& ctx, const Args&... args) {
        Logger::getInstance().warn(ctx, args...);
    }

    template<typename... Args>
    inline void error(const std::string& ctx, const Args&... args) {
        Logger::getInstance().error(ctx, args...);
    }

    template<typename... Args>
    inline void fatal(const std::string& ctx, const Args&... args) {
        Logger::getInstance().fatal(ctx, args...);
    }
}

} 

#endif // LOGGER_H