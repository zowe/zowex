#include "hello.h"
#include "zmetal.h"
#include <string>

int hello_ebcdic(std::string name, std::string& message) {
    char buffer[64];
    int bufsize = sizeof(buffer);
    HELLO(name.c_str(), buffer, &bufsize);
    message += buffer;
    return 0;
}
