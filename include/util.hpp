#pragma once
#include <string>
#include <unordered_set>
using namespace std;

string generateDuplicateName(const string& filename, const string& folderPath, unordered_set<std::string>& existingNames);