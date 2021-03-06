#pragma once

#include <math.h>
#include <thread>

double factorial(double num) {
    if (num < 0) return 0;
    if (num == 0) return 1;
    else return num * factorial(num - 1);
}

double mysqrt(double num) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    return sqrt(num);
}