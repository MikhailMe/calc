#pragma once

#include <vector>

#define EXIT      "shutdown"
#define COUNT     "count"
#define TEXT      "text"
#define SQRT      "sqrt"
#define FACTORIAL "fact"

const size_t BUFFER_SIZE = 512;
const size_t PORT = 7777;
const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 100;

struct Count_info {
    int desc_sock;
    double digit1;
    double digit2;
    std::string operation;
    bool isSlow;
};

pthread_mutex_t mutex;
std::unordered_map<int, pthread_t> clients;
std::unordered_map<int, bool> client_states;