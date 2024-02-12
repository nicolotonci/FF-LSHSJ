#include <string>
#include <fstream>
#include <sstream>
#include <vector>

std::vector<std::string> parseConfigLine(const std::string& line, char delimiter = ',') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(line);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}


std::vector<std::vector<std::string>> parseConfigFile(const std::string& filename, char delimiter = ',') {
    std::vector<std::vector<std::string>> data;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return data;
    }

    std::string line;
    while (std::getline(file, line)) {
        data.push_back(parseCSVLine(line, delimiter));
    }

    file.close();

    return data;
}
