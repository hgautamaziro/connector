#pragma once
#include <vector>
#include <string>
#include "fileInfo.hpp"
#define NO_OF_RETRIES        3

struct DownloadContext {
    std::ofstream* file;
    size_t totalDownloaded = 0;
    size_t expectedSize = 0;
};
 
class source {
public:
    source(const std::string& token);
    std::vector<fileInfo> listFilesRecursivefromOneDrive(std::string folderId);
    bool downloadFile(const std::string& id, std::string& data, size_t size);
 
private:
    std::string token;
};