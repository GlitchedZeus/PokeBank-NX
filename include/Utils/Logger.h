#ifndef UTILS_LOGGER_H
#define UTILS_LOGGER_H

#include <string>

#define LOG_BUFFER_SIZE 1024

namespace Utils {
    void logInfoToFile(const char *message);
    void logErrorToFile(const char *message);
    void logErrorToFile(std::string message);
    void logInfoToFile(const char *message, const char *context);
    void logErrorToFile(const char *message, const char *context);
    void cleanupOldLogs();
}

#endif