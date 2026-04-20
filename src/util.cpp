#include "util.hpp"
#include <string>
#include<chrono>
using namespace std;

std::string generateDuplicateName(const std::string& filename, std::unordered_set<std::string>& existingNames)
{
    if (existingNames.find(filename) == existingNames.end()) {
        existingNames.insert(filename);
        return filename;
    }
 
    int count = 1;
    std::string newName;
    size_t dotPos = filename.find_last_of('.');
    std::string name = filename.substr(0, dotPos);
    std::string ext = (dotPos != std::string::npos) ? filename.substr(dotPos) : "";
 
    while (true) {
        newName = name + "_" + std::to_string(count) + ext;
        if (existingNames.find(newName) == existingNames.end()) {
            existingNames.insert(newName);
            return newName;
        }
        count++;
    }
}