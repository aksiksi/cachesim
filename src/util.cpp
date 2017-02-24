#include <iostream>

#include "util.hpp"

void exit_on_error(std::string msg) {
    std::cout << "Error: " << msg << std::endl;
    exit(EXIT_FAILURE);
}
