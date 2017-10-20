#pragma once

#define EXIT "shutdown"
#define COUNT "count"
#define TEXT "text"
#define SQRT "sqrt"
#define FACTORIAL "fact"

const size_t BUFFER_SIZE = 512;
const size_t PORT = 32000;
const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 100;

std::mutex mutex;
std::mutex client_mutex;
std::unordered_map<int, std::thread> clients;
std::unordered_map<int, bool> client_states;