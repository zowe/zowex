#include "example.h"
#include <string>
#include <unistd.h>

extern "C" {
    #include "hello.h"
}

std::string hello_ascii(std::string name) {
    __a2e_s(&name[0]);
    std::string message;
    hello_ebcdic(name, message);
    __e2a_s(&message[0]);
    return message;
}
