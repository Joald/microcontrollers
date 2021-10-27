#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <iostream>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
int main() {
    auto fd = open("/dev/ttyACM0", O_RDWR);
    char buf[2048];

    while (true) {
        char to_send;
        std::cout << "Input your character:" << std::endl;
        std::cin >> to_send;
        auto bytes_sent = write(fd, &to_send, 1);
        if (bytes_sent < 0) {
            std::cout << "Error writing to board: " << strerror(errno) << std::endl;
            continue;
        } else {
            std::cout << "Sent " << bytes_sent << " bytes" << std::endl;
        }
        char to_receive;
        auto bytes_read = read(fd, &to_receive, 1);
        if (bytes_read < 0) {
            std::cout << "Error reading from board: " << strerror(errno) << std::endl;

        } else if (bytes_read == 0) {
            std::cout << "Board sent nothing." << std::endl;

        } else {
            std::cout << "Board sent " << bytes_read << " bytes: \"" << to_receive << "\" (" << std::hex << static_cast<int>(to_receive) << std::dec << ")\n";
        }

    }
}

#pragma clang diagnostic pop