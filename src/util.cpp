#include "util.hpp"
#include <string>
#include<chrono>
using namespace std;

string generateDuplicateName(const string& filename, const string& folderPath, unordered_set<string>& existingNames)
{
    std::string blobName = folderPath.empty() ? filename : folderPath + "_" + filename;
    for (char& c : blobName) {
       if (c == '/') c = '_';
    }

    if (existingNames.find(blobName) == existingNames.end()) {
       existingNames.insert(blobName);
       return blobName;
   }

   size_t dotPos = blobName.find_last_of('.');
   string name = blobName.substr(0, dotPos);
   string ext  = (dotPos != string::npos) ? blobName.substr(dotPos) : "";
   int count = 1;
   while (true) {
       string newName = name + "_" + to_string(count) + ext;
       if (existingNames.find(newName) == existingNames.end()) {
           existingNames.insert(newName);
           return newName;
       }
       count++;
   }
}