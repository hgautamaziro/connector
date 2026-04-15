#include <iostream>
#include <string>
#include "backupEngine.hpp"
#include "logger.hpp" 
#include "config.hpp"
using namespace std;

int main(){
    try 
    {
        AppConfig config;
        if (!ConfigLoader::load("config.json", config)) {
           LOG_ERROR("Failed to load config — exiting");
           return -1;
        }

       BackupEngine engine(config.token, config.sasUrl);
       engine.start();
       LOG_INFO("Backup process completed successfully");
    }
    catch (const std::exception& e) {
        LOG_ERROR("Unhandled exception: " + std::string(e.what()));
        return -1;
    }
    catch (...) {
        LOG_ERROR("Unknown error occurred");
        return -1;
    }
    LOG_INFO("Application exiting");
    return 0;
}