#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <sstream>

#include "constants.h"
#include "slow_operations.h"
static int request_number = 0;
static int response_number = 0;

// читаем столько, сколько пришло
ssize_t readn(int socket, char *message, size_t length, int flags) {
    ssize_t received = 0;
    while (received < length) {
        char buffer[length];
        ssize_t size = recv(socket, buffer, length, flags);
        if (size <= 0) return size;
        for (int i = 0; i < size; i++)
            message[i + received] = buffer[i];
        received += size;
    }
    return received;
}

// выводим всех подключенных клиентов
void list() {
    if (clients.empty())
        std::cout << "List if clients is empty" << std::endl;
    else {
        std::cout << "List of clients: ";
        for (auto &&client : clients)
            std::cout << client.first << " ,";
    }
    std::cout << std::endl;
}

// корректный ввод дескриптора сокета
int enter() {
    int desc_sock;
    while (true) {
        std::cout << "Enter id of client: ";
        std::cin >> desc_sock;
        if (!std::cin) {
            std::cout << "Please enter valid id of client" << std::endl;
            std::cin.clear();
            while (std::cin.get() != '\n');
        } else if (clients.count(desc_sock) > 0) {
            break;
        }
    }
    return desc_sock;
}

// пишем клиенту по дескриптору сокета
void write(int desc_sock) {
    std::string message;
    std::cout << "Enter message to " << desc_sock << " client: ";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, message);
    send(desc_sock, message.c_str(), BUFFER_SIZE, 0);
    std::cout << "Your message sent" << std::endl;
}

// пишем сообщение определённому клиенту
void write() {
    int desc_sock = enter();
    if (clients.count(desc_sock) > 0)
        write(desc_sock);
    else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

// убиваем определённого клиента
void kill() {
    int desc_sock = enter();
    pthread_mutex_lock(&mutex);
    if (client_states.count(desc_sock) > 0)
        client_states[desc_sock] = true;
    pthread_mutex_unlock(&mutex);
    std::cout << "You kill " << desc_sock << " client";
}

// убиваем всех клиентов
void killall() {
    pthread_mutex_lock(&mutex);
    for (auto &&client : clients)
        client_states[client.first] = true;
    pthread_mutex_unlock(&mutex);
    std::cout << "All clients are disabled" << std::endl;
}

// вывод отосланной и принятой информации клиента
void printServer(int desc_sock, char request[], char response[]) {
    std::cout << "Client's " << desc_sock << " request #" << ++request_number << ": " << request << std::endl;
    std::cout << "txtServer's response #" << ++response_number << ": " << response << "\n" << std::endl;
}

// обработка "быстрого" входного примера
std::string input_fast_processing(double a, const std::string &operation, double b) {
    double result = 0;
    if (operation == "+")
        result = a + b;
    else if (operation == "-")
        result = a - b;
    else if (operation == "*")
        result = a * b;
    else if (operation == "/")
        result = a / b;
    else std::cout << "incorrect operation" << std::endl;
    std::stringstream ss;
    ss << result;
    return ss.str();
}

// обработка "медленного" входного примера
std::string input_slow_processing(double a, const std::string &operation) {
    double result = 0;
    std::stringstream ss;
    if (operation == FACTORIAL)
        result = factorial(a);
    else if (operation == SQRT)
        result = mysqrt(a);
    ss << result;
    return ss.str();
}

void *t_text(int desc_sock, char *text) {
    ssize_t read_size = readn(desc_sock, text, BUFFER_SIZE, 0);
    if (read_size <= 0)return nullptr;
    send(desc_sock, text, BUFFER_SIZE, 0);
    printServer(desc_sock, text, text);
}

Count_info *read_count(int desc_sock, char *buffer) {
    // создали структуру, в которую считаем данные для вычислений клиента
    auto *ready_to_process = new Count_info();
    ready_to_process->isSlow = true;
    ready_to_process->desc_sock = desc_sock;
    // прочитаем первую чиселку
    ssize_t read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
    if (read_size <= 0) return nullptr;
    ready_to_process->digit1 = std::stoi(buffer);
    // прочитаем операцию
    read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
    if (read_size <= 0) return nullptr;
    std::string operation = buffer;
    ready_to_process->operation = operation;
    // если операция '+', '-', '*', '/', то надо прочитать вторую чиселку
    // если операция factorial или sqrt, то можно сразу отправлять на вычисление
    if (buffer[0] == '+' || buffer[0] == '-' || buffer[0] == '*' || buffer[0] == '/') {
        // прочитаем вторую чиселку
        read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0) return nullptr;
        ready_to_process->digit2 = std::stoi(buffer);
        ready_to_process->isSlow = false;
        // FIXME
    } else ready_to_process->digit2 = -52303334;
    return ready_to_process;
}

void *processing(void *count_inf) {
    // закастили структуру
    auto *count_info = (Count_info *) count_inf;
    // строка для ответа
    std::string response;
    // FIXME сохраняем номер запроса
    int temp_response_number = response_number;
    // в зависимости от того какого типа операция, выполняем считывание аргументов для вычислений
    if (count_info->isSlow) {
        // формируем запрос по струкуре @count_info для долгих операций
        std::stringstream ss;
        ss << count_info->operation << "(" << count_info->digit1 << ")";
        std::string request = ss.str();
        std::cout << "\nClient " << count_info->desc_sock
                  << " send request#" << ++request_number << ": " << request << std::endl;
        // формируем ответ по запросу
        response = input_slow_processing(count_info->digit1, count_info->operation);
        // отправляем ответ на клиенту
        send(count_info->desc_sock, response.c_str(), BUFFER_SIZE, 0);
    } else {
        // FIXME
        ++temp_response_number;
        // формируем запрос по струкуре @count_info для быстрых операций
        std::stringstream ss;
        ss << count_info->digit1 << " " << count_info->operation << " " << count_info->digit2;
        std::string request = ss.str();
        std::cout << "Client " << count_info->desc_sock
                  << " send request#" << ++request_number << ": " << request << std::endl;
        // формируем ответ по запросу
        response = input_fast_processing(count_info->digit1, count_info->operation, count_info->digit2);
        // отправляем ответ на клиенту
        send(count_info->desc_sock, response.c_str(), BUFFER_SIZE, 0);
    }
    // FIXME
    std::cout << "Server's response#" << temp_response_number << ": " << response << "\n" << std::endl;
    pthread_exit(nullptr);
}


// хэндлер каждого клиента
void *connection_handler(void *sock) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&desc_sock = (int) (intptr_t) sock;
    // отдельный поток для вычислений
    pthread_t count_thread = 0;
    // цикл на чтение команд от клиента
    while (true) {
        char buffer[BUFFER_SIZE];
        // считываем команду, подаваемую клиентом
        ssize_t read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
        // инициализация структурты для pthread_create
        if (read_size <= 0 || strcmp(buffer, EXIT) == 0) {
            pthread_mutex_lock(&mutex);
            client_states[desc_sock] = true;
            pthread_mutex_unlock(&mutex);
            break;
        } else if (strcmp(buffer, TEXT) == 0) {
            t_text(desc_sock, buffer);
        } else if (strcmp(buffer, COUNT) == 0) {
            auto *count = read_count(desc_sock, buffer);
            pthread_create(&count_thread, nullptr, &processing, count);
        }
    }
    pthread_join(count_thread, nullptr);
    close(desc_sock);
    pthread_exit(nullptr);
}

// бегает по мапе @client_states и удаляет клиентов, если отключились через 0.01 секунды
// работает всегда
void cleanup() {
    pthread_mutex_lock(&mutex);
    auto &&it = std::begin(client_states);
    while (it != std::end(client_states)) {
        auto &&entry = *it;
        auto &&fd = entry.first;
        auto &&needClose = entry.second;
        if (needClose) {
            // закрываем сокет
            shutdown(fd, SHUT_RDWR);
            close(fd);
            // находим поток в основной паме по дескиптору сокета
            auto &&thread = clients.find(fd)->second;
            // джойним потоу
            pthread_join(thread, nullptr);
            std::cout << "\nClient " << fd << " disconnected" << std::endl;
            // удаляем из мапы состяний
            it = client_states.erase(it);
            // удаляем из основной мапы
            clients.erase(fd);
            continue;
        }
        ++it;
    }
    pthread_mutex_unlock(&mutex);
}

// если произошла исключительная ситуация, пишем сообщение об ошибке и закрываем сокет
void closing(int server_socket, const char *closing_message) {
    perror(closing_message);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
}
