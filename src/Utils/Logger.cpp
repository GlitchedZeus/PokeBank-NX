#include <string>

#include <switch.h>
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>

#include "Utils/Logger.h"

namespace Utils {
    #define LOG_DIRECTORY "sdmc:/PKSE"
    #define LOG_FILE_PATH "sdmc:/PKSE/debug.log"

    void logInfoToFile(const char *message)
    {
        // Ensure the directory exists
        if (mkdir(LOG_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            // If the directory cannot be created and doesn't already exist, log to console and return
            printf("Failed to create log directory: %s\n", LOG_DIRECTORY);
            consoleUpdate(NULL);
            return;
        }

        // Open the log file in append mode
        FILE *logFile = fopen(LOG_FILE_PATH, "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);

            // Write the timestamp and message to the log file
            fprintf(logFile, "[%s][INFO] %s\n", timeBuffer, message);
            fclose(logFile);
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }

    void logErrorToFile(const char *message)
    {
        // Ensure the directory exists
        if (mkdir(LOG_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            // If the directory cannot be created and doesn't already exist, log to console and return
            printf("Failed to create log directory: %s\n", LOG_DIRECTORY);
            consoleUpdate(NULL);
            return;
        }

        // Open the log file in append mode
        FILE *logFile = fopen(LOG_FILE_PATH, "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);

            // Write the timestamp and message to the log file
            fprintf(logFile, "[%s][ERROR] %s\n", timeBuffer, message);
            fclose(logFile);
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }

    void logErrorToFile(std::string message)
    {
        // Ensure the directory exists
        if (mkdir(LOG_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            // If the directory cannot be created and doesn't already exist, log to console and return
            printf("Failed to create log directory: %s\n", LOG_DIRECTORY);
            consoleUpdate(NULL);
            return;
        }

        // Open the log file in append mode
        FILE *logFile = fopen(LOG_FILE_PATH, "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);

            // Write the timestamp and message to the log file
            fprintf(logFile, "[%s][ERROR] %s\n", timeBuffer, message.c_str());
            fclose(logFile);
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }

    void logInfoToFile(const char *message, const char *context)
    {
        // Ensure the directory exists
        if (mkdir(LOG_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            // If the directory cannot be created and doesn't already exist, log to console and return
            printf("Failed to create log directory: %s\n", LOG_DIRECTORY);
            consoleUpdate(NULL);
            return;
        }

        // Open the log file in append mode
        FILE *logFile = fopen(LOG_FILE_PATH, "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);

            // Write the timestamp and message to the log file
            fprintf(logFile, "[%s][INFO] %s: %s\n", timeBuffer, message, context);
            fclose(logFile);
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }

    void logErrorToFile(const char *message, const char *context)
    {
        // Ensure the directory exists
        if (mkdir(LOG_DIRECTORY, 0777) != 0 && errno != EEXIST)
        {
            // If the directory cannot be created and doesn't already exist, log to console and return
            printf("Failed to create log directory: %s\n", LOG_DIRECTORY);
            consoleUpdate(NULL);
            return;
        }

        // Open the log file in append mode
        FILE *logFile = fopen(LOG_FILE_PATH, "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);

            // Write the timestamp and message to the log file
            fprintf(logFile, "[%s][ERROR] %s: %s\n", timeBuffer, message, context);
            fclose(logFile);
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", LOG_FILE_PATH);
        }
    }
}