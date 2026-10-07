#include <string>

#ifdef __SWITCH__
#include <switch.h>   // not actually used below; kept for console builds
#endif
#include <time.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>

#include "Utils/Logger.h"
#include "Globals.h"
#include "Utils/PokeBankPaths.h"

//---------------------------------------------------------------------------------------------
// SD-card logging is a RUNTIME setting: Settings -> "Enable Debug Logging" (g_debugLogging in
// Globals.h, persisted to settings.cfg), default OFF. A normal run must not leave a dated debug log
// on the user's card every time they open the app, nor grow trace.log forever -- that is a
// diagnostic for development, hardware test passes and bug reports, not something every user wants.
//
// This replaced a compile-time switch (-DPKSE_PROD, from `make ... prod`). The define worked, but a
// release was then a DIFFERENT BINARY from the one that had been tested, switching modes needed a
// `make clean` because objects did not depend on the flag, and -- worst -- a user who hit a bug on
// a release had no way to produce a log at all. One binary with a toggle fixes all three: the thing
// that ships is the thing that was tested, and "turn on debug logging and reproduce it" is now a
// two-button instruction instead of "build your own .nro".
//
// EVERY sink checks the flag, so there is still exactly one thing to reason about, and a new sink
// added below without the check is the one way to get this wrong. With the flag off nothing here
// creates, writes or deletes a file.
//
// Call sites still build their argument strings when logging is off -- only the file I/O is
// skipped, which is exactly what the old define did too (it never eliminated the arguments either).
// The DECLARATIONS in Logger.h are unchanged, so all ~200 call sites and the host-harness stubs are
// unaffected by this change.
//---------------------------------------------------------------------------------------------

namespace Utils {
    static const std::string& logDirectory() { static const std::string p = PokeBank::Paths::logsRoot(); return p; }

    constexpr const char *LOG_TYPE_INFO = "INFO";
    constexpr const char *LOG_TYPE_ERROR = "ERROR";
    constexpr const char *LOG_TYPE_EVENT = "EVENT";

    // One flat file, next to the backups rather than inside logs/, so it is obvious and easy to
    // grab over MTP without digging through dated debug logs.
    static const std::string& testTracePath() { static const std::string p = PokeBank::Paths::traceFile(); return p; }

    void logTest(const std::string& line) {
        if (!g_debugLogging) return;
        std::string pathError;
        if (!PokeBank::Paths::ensureLogsRoot(&pathError)) return;
        FILE* f = fopen(testTracePath().c_str(), "a");
        if (!f) return;   // tracing must never be able to break the app it is observing
        char timeBuffer[16];
        const time_t now = time(NULL);
        strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", localtime(&now));
        fprintf(f, "[%s] %s\n", timeBuffer, line.c_str());
        fclose(f);
    }

    void logTestSession(const std::string& details) {
        if (!g_debugLogging) return;
        std::string pathError;
        if (!PokeBank::Paths::ensureLogsRoot(&pathError)) return;
        FILE* f = fopen(testTracePath().c_str(), "a");
        if (!f) return;
        char dateBuffer[32];
        const time_t now = time(NULL);
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d %H:%M:%S", localtime(&now));
        // A visible separator so several sessions in one file stay readable, and so a reader can
        // tell at a glance where a run began rather than inferring it from timestamps.
        fprintf(f, "\n========================================================================\n");
        fprintf(f, "SESSION  %s  %s\n", dateBuffer, details.c_str());
        fprintf(f, "========================================================================\n");
        fclose(f);
    }

    static std::string getCurrentLogFilePath() {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);

        char dateBuffer[16];
        strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d", t);

        return logDirectory() + "/debug_" + dateBuffer + ".log";
    }
    // For internal use only. Every logInfoToFile/logErrorToFile overload funnels through here, so
    // this one check covers all of them -- the wrappers below deliberately do not repeat it.
    void logToFile(const char *type, const char *message, const char *context = NULL) {
        if (!g_debugLogging) return;

        const bool hasContext = (context != NULL && context[0] != '\0');

        std::string pathError;
        if (!PokeBank::Paths::ensureLogsRoot(&pathError)) {
            printf("Failed to create PokeBank NX log directory: %s\n", pathError.c_str());
#ifdef __SWITCH__
            consoleUpdate(NULL);
#endif
            return;
        }

        std::string logFilePath = getCurrentLogFilePath();

        // Open the log file in append mode
        FILE *logFile = fopen(logFilePath.c_str(), "a");
        if (logFile)
        {
            // Get the current time
            time_t now = time(NULL);
            struct tm *t = localtime(&now);

            // Format the timestamp (e.g., "YYYY-MM-DD HH:MM:SS")
            char timeBuffer[20];
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", t);
            if (hasContext) {
                fprintf(logFile, "[%s][%s] %s: %s\n", timeBuffer, type, message, context);
                fclose(logFile);
            }
            else {
                fprintf(logFile, "[%s][%s] %s\n", timeBuffer, type, message);
                fclose(logFile);
            }
        }
        else
        {
            // Log to console if the file cannot be opened
            printf("Failed to open log file: %s\n", logFilePath.c_str());
        }
    }

    void logInfoToFile(const char *message)
    {
        logToFile(LOG_TYPE_INFO, message);
    }

    void logInfoToFile(std::string message)
    {
        logToFile(LOG_TYPE_INFO, message.c_str());
    }

    void logErrorToFile(const char *message)
    {
        logToFile(LOG_TYPE_ERROR, message);
    }

    void logErrorToFile(std::string message)
    {
        logToFile(LOG_TYPE_ERROR, message.c_str());
    }

    void logInfoToFile(const char *message, const char *context)
    {
        logToFile(LOG_TYPE_INFO, message, context);
    }

    void logErrorToFile(const char *message, const char *context)
    {
        logToFile(LOG_TYPE_ERROR, message, context);
    }

    void logEventToFile(const std::string& line)
    {
        logToFile(LOG_TYPE_EVENT, line.c_str());
    }

    void cleanupOldLogs()
    {
        // Gated with the sinks, on the same reasoning the old production build used: a run that
        // writes no logs has no business deleting files either. Turning logging off freezes the log
        // directory as it stands rather than quietly pruning it out from under the user -- whatever
        // is on the card was put there deliberately, and removing it is their call.
        if (!g_debugLogging) return;

        DIR* dir = opendir(logDirectory().c_str());
        if (!dir) {
            return; // Directory doesn't exist or can't be opened
        }

        time_t now = time(NULL);
        time_t retentionTime = LOG_RETENTION_DAYS * 24 * 60 * 60; // Convert days to seconds

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Check if filename starts with "debug_" and ends with ".log"
            std::string filename(entry->d_name);
            if (filename.find("debug_") == 0 && filename.find(".log") == filename.length() - 4) {
                // Build full path
                std::string fullPath = logDirectory() + "/" + filename;

                // Get file modification time
                struct stat fileInfo;
                if (stat(fullPath.c_str(), &fileInfo) == 0) {
                    // Calculate file age in seconds
                    time_t fileAge = now - fileInfo.st_mtime;

                    // Delete if older than retention period
                    if (fileAge > retentionTime) {
                        unlink(fullPath.c_str());
                    }
                }
            }
        }

        closedir(dir);
    }
}