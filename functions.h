#pragma once

#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <sstream>

#include "constants.h"
#include "fast_operations.h"
#include "slow_operations.h"

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
        } else if (clients.count(desc_sock) > 0) break;
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

// убиваем клиента по дескриптору сокета
void kill(int desc_sock) {
    if (clients.count(desc_sock) > 0) {
        shutdown(desc_sock, SHUT_RDWR);
        close(desc_sock);
        auto &&thread = clients.find(desc_sock)->second;
        if (thread.joinable())
            thread.join();
        std::cout << "\nClient " << desc_sock << " disconnected" << std::endl;
    } else std::cout << "Sorry, but this client doesn't exist" << std::endl;
}

// говорим, что необходимо удалить desc_sock
void lazy_kill(int desc_sock) {
    if (client_states.count(desc_sock) > 0)
        client_states[desc_sock] = true;
}

// убиваем определённого клиента
void kill() {
    int desc_sock = enter();
    std::unique_lock<std::mutex> lock(client_mutex);
    lazy_kill(desc_sock);
    std::cout << "You kill " << desc_sock << " client";
}

// убиваем всех клиентов
void killall() {
    std::unique_lock<std::mutex> lock(client_mutex);
    for (auto &&client : clients)
        lazy_kill(client.first);
    std::cout << "All clients are disabled" << std::endl;
}

// вывод всех команд, которые можно использовать на сервере
void server_help() {
    std::cout << "You can use the following commands:" << std::endl;
    std::cout << "list     -  show list of all clients" << std::endl;
    std::cout << "write    -  send message to certain client" << std::endl;
    std::cout << "kill     -  remove certain client" << std::endl;
    std::cout << "killall  -  remove all clients" << std::endl;
    std::cout << "shutdown -  server shutdown" << std::endl;
}

// вывод всех команд, которые можно использовать на клиенте
void client_help() {
    std::cout << "Hello from server!" << std::endl;
    std::cout << "text - write any message to server" << std::endl;
    std::cout << "count - send to server math example" << std::endl;
    std::cout << "shutdown - disconnect from the server" << std::endl;
}

// вывод отосланной и принятой информации клиента
//void printServer(int desc_sock, char request[], char response[]) {
void printServer(int desc_sock, char request[], char response[]) {
    std::cout << "Client " << desc_sock << " send request: " << request << std::endl;
    std::cout << "Server's response: " << response << std::endl;
}

// обработка "быстрого" входного примера
std::string input_fast_proccssing(int a, char sing, int b) {
    double result = 0;
    switch (sing) {
        case '+':
            result = addition(a, b);
            break;
        case '-':
            result = subtraction(a, b);
            break;
        case '*':
            result = multiplication(a, b);
            break;
        case '/':
            result = divide(a, b);
            break;
        default:
            std::cout << "incorrect operation" << std::endl;
            break;
    }
    std::stringstream ss;
    ss << result;
    return ss.str();
}

// обработка "медленного" входного примера
std::string input_slow_proccssing(int a, char action[]) {
    const std::string FACT = "fact";
    const std::string SQRT = "sqrt";
    double result = 0;
    std::stringstream ss;
    if (action == FACT) {
        result = (double) factorial(a);
    } else if (action == SQRT) {
        result = mysqrt(a);
    }
    ss << result;
    return ss.str();
}

// хэндлер каждого клиента
void connection_handler(int desc_sock) {
    while (true) {
        char buffer[BUFFER_SIZE];
        ssize_t read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
        if (read_size <= 0 || strcmp(buffer, EXIT) == 0)
            break;
        else if (strcmp(buffer, COUNT) == 0) {
            // первая чиселка
            read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
            if (read_size <= 0) break;
            std::string digit_one(buffer);
            int a = std::stoi(digit_one);
            // знак
            read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
            if (read_size <= 0) break;
            // смотрим какая операция: быстрая или долгая
            if (buffer[0] == '+' || buffer[0] == '-' || buffer[0] == '*' || buffer[0] == '/') {
                char sign = buffer[0];
                // вторая чиселка
                read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
                if (read_size <= 0) break;
                std::string digit_two(buffer);
                int b = std::stoi(digit_two);
                // делаем строку-пример
                std::stringstream ss;
                ss << a << sign << b;
                std::string request = ss.str();
                // формирование вывода на экран
                std::cout << "Client " << desc_sock << " send request: " << request << std::endl;
                std::string response = input_fast_proccssing(a, sign, b);
                std::cout << "Server's response: " << response << std::endl;
                send(desc_sock, response.c_str(), BUFFER_SIZE, 0);
            } else if (strcmp(buffer, SQRT) == 0 || strcmp(buffer, FACTORIAL) == 0) {
                std::stringstream ss;
                ss << buffer << "(" << a << ")";
                std::string request = ss.str();
                std::cout << "Client " << desc_sock << " send request: " << request << std::endl;
                std::string response = input_slow_proccssing(a, buffer);
                std::cout << "Server's response: " << response << std::endl;
                send(desc_sock, response.c_str(), BUFFER_SIZE, 0);
            }
        } else if (strcmp(buffer, TEXT) == 0) {
            read_size = readn(desc_sock, buffer, BUFFER_SIZE, 0);
            if (read_size <= 0) break;
            send(desc_sock, buffer, BUFFER_SIZE, 0);
            printServer(desc_sock, buffer, buffer);
        }
    }
    close(desc_sock);
}

// если клиент сам отключился, то удаляем его из двух мап через 0,01 секунды
// работает всегда
void cleanup() {
    std::unique_lock<std::mutex> lock(client_mutex);
    auto &&it = std::begin(client_states);
    while (it != std::end(client_states)) {
        auto &&entry = *it;
        auto &&fd = entry.first;
        auto &&needClose = entry.second;
        if (needClose) {
            kill(fd);
            it = client_states.erase(it);
            clients.erase(fd);
            continue;
        }
        ++it;
    }
}

// закрываем сокет при неудаче
void closing(int server_socket, const char *closing_message) {
    perror(closing_message);
    shutdown(server_socket, SHUT_RDWR);
    close(server_socket);
}

// хэдлер сервера
void server_handler(int server_socket) {
    int new_socket;
    while (true) {
        // задаёем таймаут
        timeval timeout{TIMEOUT_SEC, TIMEOUT_USEC};
        // задаём множество сокетов
        fd_set server_sockets{};
        FD_ZERO(&server_sockets);
        FD_SET(server_socket, &server_sockets);
        // выбираем сокет, у которого возник какой-то ивент
        auto &&result = select(server_socket + 1, &server_sockets, nullptr, nullptr, &timeout);
        cleanup();
        if (result < 0) {
            closing(server_socket, "Select failed");
            break;
        }
        if (result == 0 || !FD_ISSET(server_socket, &server_sockets))
            continue;
        // асептим новый сокет
        new_socket = accept(server_socket, nullptr, nullptr);
        if (new_socket < 0) {
            closing(server_socket, "Accept failed");
            break;
        }
        std::cout << "\nConnection accepted" << std::endl;
        std::cout << "In list we have: " << clients.size() + 1 << std::endl;
        // так как new_socket пока не нужно убивать ставим false
        {
            std::unique_lock<std::mutex> lock(client_mutex);
            client_states[new_socket] = false;
        }
        // добавляем клиентов в мапу и даём каждому поток
        {
            std::unique_lock<std::mutex> lock(mutex);
            clients.emplace(new_socket, [new_socket]() -> void {
                try {
                    connection_handler(new_socket);
                } catch (...) {}
                // необходимо убить new_socket, поэтому ставим true
                client_states[new_socket] = true;
            });
        }
    }
}

// хэндлер клиента для чтения в отдельном потоке
void client_read_handler(int desc_sock) {
    char buffer[BUFFER_SIZE + 1];
    while (true) {
        bzero(buffer, BUFFER_SIZE + 1);
        // слушаем сервер
        if (readn(desc_sock, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("Readn failed");
            close(desc_sock);
            break;
        }
        // если сервер прислал "shutdown", то умираем
        if (strcmp(buffer, EXIT) == 0)
            break;
        std::cout << "Server's response: " << buffer << std::endl;
    }
    close(desc_sock);
}

//  отправляем пример серверу
void count(int client_socket) {
    std::string message = COUNT;
    send(client_socket, message.c_str(), BUFFER_SIZE, 0);
    std::string temp;
    // первая чиселка
    std::cout << "Enter first digit: ";
    std::getline(std::cin, message);
    temp += message;
    temp += " ";
    send(client_socket, message.c_str(), BUFFER_SIZE, 0);
    // знак
    std::cout << "Enter sign: ";
    std::getline(std::cin, message);
    temp += message;
    temp += " ";
    send(client_socket, message.c_str(), BUFFER_SIZE, 0);
    // если выполняем долгую операцию => вторая чиселка просто не нужна!
    if (strcmp(message.c_str(), SQRT) != 0 && strcmp(message.c_str(), FACTORIAL) != 0) {
        // вторая чиселка
        std::cout << "Enter second digit: ";
        std::getline(std::cin, message);
        temp += message;
        std::cout << "Client's request : " << temp << std::endl;
        send(client_socket, message.c_str(), BUFFER_SIZE, 0);
    }
}

// отправляем просто текст серверу
void text(int client_socket) {
    std::string message = TEXT;
    // отправили серверу уведомление, что сейчас будет текст
    if (send(client_socket, message.c_str(), BUFFER_SIZE, 0) < 0) {
        perror("Send error");
    }
    std::cout << "Your message to server: ";
    std::getline(std::cin, message);
    // отправляем message на сервер
    if (send(client_socket, message.c_str(), BUFFER_SIZE, 0) == -1) {
        perror("Send error");
    }
}