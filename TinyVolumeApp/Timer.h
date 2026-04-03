#pragma once
#include <chrono>
#include <stdio.h>
class Logger {
#ifdef _WIN32
#define lock_stream(f) _lock_file(f)
#define unlock_stream(f) _unlock_file(f)
#else
#define lock_stream(f) flockfile(f)
#define unlock_stream(f) funlockfile(f)
#endif

    using clock = std::chrono::high_resolution_clock;
    friend class ScopeTimer;

    FILE* _file;
    std::chrono::time_point<clock> _start;
    uint8_t _indentation;

public:
    Logger()
    {
#ifdef _WIN32
        fopen_s(&_file, "timepoints.txt", "w");
#else
        _file = fopen("timepoints.txt", "w");
#endif
        if (_file) {
            fprintf(_file, "[LOGGING START]\n");
            fflush(_file);
        }
        _indentation = 0;
        _start = clock::now();
    }

    template <bool WithIdentation = false>
    void log(const char* format, ...)
    {
        if (!_file)
            return;

        lock_stream(_file);

        if constexpr (WithIdentation)
            for (int i = 0; i < _indentation; ++i)
                fputc(' ', _file);

        va_list args;
        va_start(args, format), vfprintf(_file, format, args), va_end(args);

        unlock_stream(_file);
    }

    double getDuration() const { return std::chrono::duration<double>(clock::now() - _start).count(); }
    void reset() { _start = clock::now(); }

    ~Logger()
    {
        if (_file) {
            log("[LOGGING END]\n");
            fclose(_file);
            _file = nullptr;
        }
    }

    static Logger& get()
    {
        static Logger instance;
        return instance;
    }

// #define TIMEPOINT_RESET_OPTION , Logger::get().reset()
#define TIMEPOINT_RESET_OPTION
#define TIMEPOINT_DURATION_MS (Logger::get().getDuration() * 1000.0)
#define TIMEPOINT_DURATION_AFTER(format, ...) Logger::get().log(format "\t %.3f ms\n", ##__VA_ARGS__, TIMEPOINT_DURATION_MS) TIMEPOINT_RESET_OPTION;
#define TIMEPOINT_DURATION_BEFORE(format, ...) Logger::get().log("%.3f ms \t" format "\n", TIMEPOINT_DURATION_MS, ##__VA_ARGS__) TIMEPOINT_RESET_OPTION;

#if LOG_ENABLED == 1
#define TIMEPOINT TIMEPOINT_DURATION_BEFORE
#else
#define TIMEPOINT
#endif
};

class ScopeTimer {
    using clock = std::chrono::high_resolution_clock;
    std::chrono::time_point<clock> _start;
    const char* _timerName;

public:
    ScopeTimer(const ScopeTimer&) = delete;
    ScopeTimer(ScopeTimer&&) = delete;
    ScopeTimer(const char* str)
        : _timerName(str)
    {
        Logger::get()._indentation += 2;
        Logger::get().log<true>("%s, started", _timerName);
        _start = clock::now();
    }

    ~ScopeTimer()
    {
        const float duration = std::chrono::duration<float>(clock::now() - _start).count();
        Logger::get().log<true>("%s, finished %.3f ms\n", _timerName, duration * 1000);
        Logger::get()._indentation -= 2;
    }

#define TIMEPOINT_RAII(str) ScopeTimer scopeTimer(str);
};