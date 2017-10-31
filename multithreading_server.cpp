#include "functions.h"

// хэдлер сервера
void *server_handler(void *serv_sock) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&server_socket = (int) (intptr_t) serv_sock;
    int new_socket;
    while (true) {
        // задаёем таймаут
        timeval timeout{TIMEOUT_SEC, TIMEOUT_USEC};
        // задаём множество сокетов
        fd_set server_sockets{};
        FD_ZERO(&server_sockets); // NOLINT
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
        std::cout << "In list we have: " << clients.size() + 1 << "\n" << std::endl;
        // так как new_socket пока не нужно убивать ставим false
        pthread_mutex_lock(&mutex);
        client_states[new_socket] = false;
        pthread_mutex_unlock(&mutex);
        // добавляем клиентов в мапу и даём каждому поток
        pthread_mutex_lock(&mutex);
        pthread_t clients_thread;
        if (pthread_create(&clients_thread, nullptr, connection_handler, (void *) (intptr_t) new_socket) == 0)
            clients.emplace(new_socket, clients_thread);
        else client_states[new_socket] = true;
        pthread_mutex_unlock(&mutex);

    }
    pthread_exit(nullptr);
}

// вывод всех команд, которые можно использовать на сервере
void server_help() {
    std::cout << "*****************************************************" << std::endl;
    std::cout << "*   Hello, I'm the greatest server ever!            *" << std::endl;
    std::cout << "*                                                   *" << std::endl;
    std::cout << "*   You can use the following commands:             *" << std::endl;
    std::cout << "*    1) list     -  show list of all clients        *" << std::endl;
    std::cout << "*    2) write    -  send message to certain client  *" << std::endl;
    std::cout << "*    3) kill     -  remove certain client           *" << std::endl;
    std::cout << "*    4) killall  -  remove all clients              *" << std::endl;
    std::cout << "*    5) shutdown -  server shutdown                 *" << std::endl;
    std::cout << "*****************************************************" << std::endl;
}

int main() {
    // дескрипторы сокетов
    int server_socket;
    struct sockaddr_in addr{};
    // создали сокет, который будет слушать
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Could not create socket");
        return -1;
    }
    // задали адрес и порт
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    // для того, чтобы порт не был занят
    int yes = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    // привязали сокет к адресу
    if (bind(server_socket, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
        perror("Bind failed");
        return -2;
    }
    // server ждёт запросов со стороны клиентов
    if (listen(server_socket, 3) != 0) {
        close(server_socket);
        perror("Listen error");
        return -3;
    }
    std::cout << "Server started on port " << PORT << std::endl;
    std::cout << "Waiting for incoming connections..." << std::endl;
    // вывод команд, доступных для сервера
    server_help();
    // создаём отдельный поток для сервера
    pthread_t server_thread;
    // запускаем поток для сервера
    pthread_create(&server_thread, nullptr, server_handler, (void *) (intptr_t) server_socket);
    // поток для ввода команд сервера
    while (true) {
        std::string command;
        std::getline(std::cin, command);
        if (command == "list") list();
        else if (command == "write") write();
        else if (command == "kill") kill();
        else if (command == "killall") killall();
        else if (command == "shutdown") {
            killall();
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            break;
        }
    }
    pthread_join(server_thread, nullptr);
    std::cout << "Goodbye" << std::endl;
    return 0;
}
