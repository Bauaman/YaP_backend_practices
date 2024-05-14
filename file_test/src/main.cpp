#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    std::fstream file;
    file.open(std::string(argv[1]));

    if (file.is_open()) {
        std::cout << "File is open" <<std::endl;
    } else {
        std::cout << "Failed to open file" << std::endl;
        return 1;
    }

    std::streambuf *buf;
    buf = file.rdbuf();

    std::stringstream ss;
    ss << buf;
    std::string str = ss.str();

    std::cout << str << std::endl;

    file.close();
    if (file.is_open()) {
        std::cout << "File is still opened" << std::endl;
    } else {
        std::cout << "File closed OK" << std::endl;
    }

}