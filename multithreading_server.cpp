#include "functions.h"

int main() {
    // дескрипторы сокетов
    int server_socket, new_socket;
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

    // отдельный поток для сервера
    std::unique_lock<std::mutex> lock(mutex);
    std::thread server_thread([server_socket]() -> void {
        server_handler(server_socket);
    });
    lock.unlock();

    std::string command;

    while (true) {
        std::getline(std::cin, command);
        if (command == "list") list();
        else if (command == "write") write();
        else if (command == "kill") kill();
        else if (command == "killall") {
            killall();
            std::cout << "All clients are disabled" << std::endl;
        }
        else if (command == "shutdown") {
            killall();
            std::cout << "All clients are disabled" << std::endl;
            shutdown(server_socket, SHUT_RDWR);
            close(server_socket);
            break;
        }
    }
    if (server_thread.joinable())
        server_thread.join();
    std::cout << "Goodbye" << std::endl;
    return 0;
}
