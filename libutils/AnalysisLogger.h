#ifndef ANALYSIS_LOGGER_H
#define ANALYSIS_LOGGER_H

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace analysis {

enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

class AnalysisLogger {
  public:
    static AnalysisLogger &getInstance() {
        static AnalysisLogger instance;
        return instance;
    }

    void setLevel(LogLevel level) {
        logger_->set_level(toSpd(level));
    }

    template <typename... Args>
    void debug(const std::string &context, const Args &...args) {
        log(LogLevel::DEBUG, context, args...);
    }

    template <typename... Args>
    void info(const std::string &context, const Args &...args) {
        log(LogLevel::INFO, context, args...);
    }

    template <typename... Args>
    void warn(const std::string &context, const Args &...args) {
        log(LogLevel::WARN, context, args...);
    }

    template <typename... Args>
    void error(const std::string &context, const Args &...args) {
        log(LogLevel::ERROR, context, args...);
    }

    template <typename... Args>
    void fatal(const std::string &context, const Args &...args) {
        log(LogLevel::FATAL, context, args...);
    }

  private:
    AnalysisLogger() {
        logger_ = spdlog::stderr_color_mt("analysis");
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
    }
    ~AnalysisLogger() = default;
    AnalysisLogger(const AnalysisLogger &) = delete;
    AnalysisLogger &operator=(const AnalysisLogger &) = delete;

    template <typename... Args>
    void log(LogLevel level, const std::string &context, const Args &...args) {
        std::ostringstream oss;
        ((oss << args << ' '), ...);
        std::string msg = oss.str();
        if (!msg.empty()) {
            msg.pop_back();
        }

        std::string combined = "[" + context + "]";
        if (!msg.empty()) {
            combined += " " + msg;
        }

        logger_->log(toSpd(level), combined);

        if (level == LogLevel::FATAL) {
            throw std::runtime_error(combined);
        }
    }

    spdlog::level::level_enum toSpd(LogLevel level) const {
        switch (level) {
        case LogLevel::DEBUG:
            return spdlog::level::debug;
        case LogLevel::INFO:
            return spdlog::level::info;
        case LogLevel::WARN:
            return spdlog::level::warn;
        case LogLevel::ERROR:
            return spdlog::level::err;
        case LogLevel::FATAL:
            return spdlog::level::critical;
        }
        return spdlog::level::info;
    }

    std::shared_ptr<spdlog::logger> logger_;
};

namespace log {
template <typename... Args>
inline void debug(const std::string &ctx, const Args &...args) {
    AnalysisLogger::getInstance().debug(ctx, args...);
}
template <typename... Args>
inline void info(const std::string &ctx, const Args &...args) {
    AnalysisLogger::getInstance().info(ctx, args...);
}
template <typename... Args>
inline void warn(const std::string &ctx, const Args &...args) {
    AnalysisLogger::getInstance().warn(ctx, args...);
}
template <typename... Args>
inline void error(const std::string &ctx, const Args &...args) {
    AnalysisLogger::getInstance().error(ctx, args...);
}
template <typename... Args>
inline void fatal(const std::string &ctx, const Args &...args) {
    AnalysisLogger::getInstance().fatal(ctx, args...);
}
} // namespace log

} // namespace analysis

#endif
