#pragma once

#include <math.h>

unsigned long factorial(int a) {
    if(a < 0) return 0;
    if (a == 0) return 1;
    else return a * factorial(a - 1);
}

double mysqrt(double a) {
    return sqrt(a);
}