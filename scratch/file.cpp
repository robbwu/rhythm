
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include "file.hpp"

// File operations
bool saveFile(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }

    file << content;
    file.close();

    if (file.fail()) {
        std::cerr << "Failed to write to file: " << filepath << std::endl;
        return false;
    }

    return true;
}

bool loadFile(const std::string& filepath, std::string& content) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filepath << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    content = buffer.str();
    file.close();

    return true;
}