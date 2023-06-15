#include <chrono>
#include <iostream>
#include <vector>

typedef std::vector<std::string> StringList;

#define stic TicTocTimerObj.start()
#define tic stic
#define stoc TicTocTimerObj.stop()
#define toc \
    stoc;   \
    TicTocTimerObj.elapsed(true, __LINE__, __FUNCTION__)
#define stictoc TicTocTimerObj.elapsed() const char* line = nullptr, const char* function = nullptr)
#define stictocer(unit, is_print)                                        \
    TicTocTimerObj.elapsed<std::chrono::unit>(is_print, #unit, __LINE__, \
                                              __FUNCTION__)
#define tictocer(unit)                                               \
    stoc;                                                            \
    TicTocTimerObj.elapsed<std::chrono::unit>(true, #unit, __LINE__, \
                                              __FUNCTION__)

struct TicTocTimer {
    typedef std::chrono::high_resolution_clock chrono_clock;
    typedef std::chrono::time_point<chrono_clock> time_point;
    typedef std::chrono::microseconds unit;
    const char* unit_name = "microseconds";
    time_point _start, _stop;
    void start() { _start = chrono_clock::now(); }
    void stop() { _stop = chrono_clock::now(); }
    double elapsed(bool is_print = false,
                   const int line = -1,
                   const char* function = nullptr) {
        const double _elapsed =
            std::chrono::duration_cast<unit>(_stop - _start).count();
        if (is_print) {
            std::cout << "Elapsed time is " << _elapsed << " " << unit_name;
            if (line > 0)
                std::cout << " in line \"" << line << "\"";
            if (function)
                std::cout << " in function \"" << function << "\"";
            std::cout << "." << std::endl;
        }
        return _elapsed;
    }
    template <typename DurationUnit>
    double elapsed(bool is_print = false,
                   const char* unit_name = "",
                   const int line = -1,
                   const char* function = nullptr) {
        const double _elapsed =
            std::chrono::duration_cast<DurationUnit>(_stop - _start).count();
        if (is_print) {
            std::cout << "Elapsed time is " << _elapsed << " " << unit_name;
            if (line > 0)
                std::cout << " in line " << line;
            if (function)
                std::cout << " in function " << function;
            std::cout << "." << std::endl;
        }
        return _elapsed;
    }
} TicTocTimerObj;
