#include "functions.h"

int main(int argc, char **argv) {
    int client_socket;
    sockaddr_in addr{};

    // задали сокет
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Can't create socket");
        return -1;
    }

    // задали адрес
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // установление соединения с сервером со стороны клиента
    if (connect(client_socket, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Сonnection error");
        return -2;
    }
    std::cout << "Client started.. Send message to server" << std::endl;
    std::string message;

    // вывод команд, доступных для клиента
    client_help();

    // отдельный поток на чтение сообщений с сервера
    std::thread client_read([client_socket]() -> void {
        client_read_handler(client_socket);
    });

    // команда дя клиента
    std::string command;

    while (true) {
        std::getline(std::cin, command);
        if (command == TEXT) text(client_socket);
        else if (command == COUNT) count(client_socket);
        else if (command == EXIT) {
            shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
            break;
        }
    }

    if (client_read.joinable())
        client_read.join();

    std::cout << "Disconnected" << std::endl;
    std::cout << "Client closed" << std::endl;
    return 0;
}
