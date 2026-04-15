#pragma once
#include <string>
#include <unordered_set>
using namespace std;

using namespace std;
//string generateUniqueBlobName(const std::string& filename);
std::string generateDuplicateName(const std::string& filename,
                                  std::unordered_set<std::string>& existingNames);