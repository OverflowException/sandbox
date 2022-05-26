#pragma once

#include <chrono>

class Timer {
public:
    inline void start() {
        t_start = std::chrono::high_resolution_clock::now();
        t0 = t_start;
    }

    inline float interval() {
        auto t1 =  std::chrono::high_resolution_clock::now();
        float interval =  std::chrono::duration_cast<std::chrono::duration<float>>(t1 - t0).count();
        t0 = t1;
        return interval;
    }

    inline float current() {
        auto now =  std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::duration<float>>(now - t_start).count();
    }

private:
    std::chrono::high_resolution_clock::time_point t_start;
    std::chrono::high_resolution_clock::time_point t0;
};