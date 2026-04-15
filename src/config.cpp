#include "config.hpp"
#include "logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
using namespace std;
using json = nlohmann::json;


bool ConfigLoader::load(const string& filePath, AppConfig& config)
{
   LOG_INFO("Loading config from: " + filePath);
   ifstream file(filePath);
   if (!file.is_open()) {
       LOG_ERROR("Cannot open config file: " + filePath);
       return false;
   }
   json j;
   try {
       file >> j;
   } catch (const exception& e) {
       LOG_ERROR("Config JSON parse failed: " + string(e.what()));
       return false;
   }
   if (!j.contains("token") || !j.contains("sasUrl")) {
       LOG_ERROR("Config missing 'token' or 'sasUrl' fields");
       return false;
   }
   config.token  = j["token"].get<string>();
   config.sasUrl = j["sasUrl"].get<string>();
   if (config.token.empty()) {
       LOG_ERROR("Token is empty in config");
       return false;
   }
   if (config.sasUrl.empty()) {
       LOG_ERROR("SAS URL is empty in config");
       return false;
   }
   LOG_INFO("Config loaded successfully");
   return true;
}