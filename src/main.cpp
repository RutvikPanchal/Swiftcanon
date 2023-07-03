#include "Swiftcanon.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>

int main() {
    Swiftcanon swiftcanon;
    
    try{
        swiftcanon.init();
        swiftcanon.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}