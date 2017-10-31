#include "functions.h"

// хэндлер клиента для чтения в отдельном потоке
void *client_read_handler(void *ds) {
    // кастим к int, к нормальному виду дескриптора сокета
    auto &&desc_sock = (int) (intptr_t) ds;
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
        std::cout << "Server's response#" << response_number << ": " << buffer << "\n" << std::endl;
    }
    close(desc_sock);
    pthread_exit(nullptr);
}

// вывод всех команд, которые можно использовать на клиенте
void client_help() {
    std::cout << "************************************************" << std::endl;
    std::cout << "*   Hello from server!                         *" << std::endl;
    std::cout << "*                                              *" << std::endl;
    std::cout << "*   You can use the following commands:        *" << std::endl;
    std::cout << "*    1) text  - write any message to server    *" << std::endl;
    std::cout << "*    2) count - send to server math example    *" << std::endl;
    std::cout << "*       Available signs:                       *" << std::endl;
    std::cout << "*          a) fact                             *" << std::endl;
    std::cout << "*          b) sqrt                             *" << std::endl;
    std::cout << "*          c) +, - , * , /                     *" << std::endl;
    std::cout << "*    3) shutdown - disconnect from the server  *" << std::endl;
    std::cout << "************************************************" << std::endl;
}

//  отправляем пример серверу
void count(int client_socket) {
    // FIXME
    int temp_request_number = request_number;
    std::string count = COUNT;
    // говорим серверу, что хотим посчитать
    send(client_socket, count.c_str(), BUFFER_SIZE, 0);
    // собираем строку-пример
    std::string computation;
    // первая чиселка
    std::string first_num;
    std::cout << "Enter first number: ";
    std::getline(std::cin, first_num);
    send(client_socket, first_num.c_str(), BUFFER_SIZE, 0);
    // операция, которую будем выполнять
    std::string operation;
    std::cout << "Enter operation: ";
    std::getline(std::cin, operation);
    send(client_socket, operation.c_str(), BUFFER_SIZE, 0);
    // если выполняем долгую операцию => вторая чиселка просто не нужна!
    if (strcmp(operation.c_str(), SQRT) != 0 && strcmp(operation.c_str(), FACTORIAL) != 0) {
        // FIXME temp_request_number = request_number;
        computation = first_num + operation + " ";
        // вторая чиселка
        std::string second_num;
        std::cout << "Enter second number: ";
        std::getline(std::cin, second_num);
        computation += second_num;
        // FIXME
        std::cout << "Client's request#" << ++request_number << ": " << computation << std::endl;
        send(client_socket, second_num.c_str(), BUFFER_SIZE, 0);
    } else {
        computation = operation + "(" + first_num + ")";
        // FIXME
        std::cout << "Client's request#" << temp_request_number << ": " << computation << std::endl;
    }
}

// отправляем просто текст серверу
void text(int client_socket) {
    std::string text = TEXT;
    // отправили серверу уведомление, что сейчас будет отправлен текст
    if (send(client_socket, text.c_str(), BUFFER_SIZE, 0) < 0)
        perror("Send error");
    // вводим сообщение для отправки на сервер
    std::string message;
    // FIXME
    std::cout << "Your message#" << ++request_number << " to server: ";
    std::getline(std::cin, message);
    // отправляем message на сервер
    if (send(client_socket, message.c_str(), BUFFER_SIZE, 0) == -1)
        perror("Send error");
}

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
    // вывод команд, доступных для клиента
    client_help();
    // отдельный поток на чтение сообщений с сервера
    pthread_t client_thread;
    pthread_create(&client_thread, nullptr, client_read_handler, (void *) (intptr_t) client_socket);
    // команды для клиента
    while (true) {
        std::string command;
        std::getline(std::cin, command);
        if (command == TEXT)
            text(client_socket);
        else if (command == COUNT)
            count(client_socket);
        else if (command == EXIT) { ;
            shutdown(client_socket, SHUT_RDWR);
            close(client_socket);
            break;
        }
    }
    // джойним поток на чтение и закрываем сокет
    pthread_join(client_thread, nullptr);
    close(client_socket);
    std::cout << "Disconnected" << std::endl;
    std::cout << "Client closed" << std::endl;
    return 0;
}
