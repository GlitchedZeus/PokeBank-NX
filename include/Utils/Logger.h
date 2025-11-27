#ifndef LOGGER_H
#define LOGGER_H

#include <string>

#define LOG_BUFFER_SIZE 1024

void logErrorToConsole(const char *message);
void logInfoToConsole(const char *message);
void logErrorToConsole(std::string message);
void logInfoToConsole(std::string message);
void logErrorToConsole(const char *message, const char *context);
void logInfoToConsole(const char *message, const char *context);
void logErrorToConsole(const char *message, std::string context);
void logInfoToConsole(const char *message, std::string context);

void logInfoToFile(const char *message);
void logErrorToFile(const char *message);
void logInfoToFile(std::string message);
void logErrorToFile(std::string message);
void logInfoToFile(const char *message, const char *context);
void logErrorToFile(const char *message, const char *context);
void logInfoToFile(const char *message, std::string context);
void logErrorToFile(const char *message, std::string context);

#endif