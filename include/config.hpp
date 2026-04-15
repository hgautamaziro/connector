#pragma once
#include <string>
using namespace std;

struct AppConfig {
   string token;
   string sasUrl;
};
class ConfigLoader {
public:
   static bool load(const string& filePath, AppConfig& config);
};