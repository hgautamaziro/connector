#pragma once
#include <vector>
#include <string>
#include "fileInfo.hpp"
using namespace std;

#define NO_OF_RETRIES        3

struct DownloadContext {
    ofstream* file;
    size_t totalDownloaded = 0;
    size_t expectedSize = 0;
};
 
class source {
public:
    source(const string& token);
    vector<fileInfo> listFilesRecursivefromOneDrive(string folderId, string parentPath="");
    bool downloadFile(const string& id, string& data, size_t size);
    bool verifyFileUnchanged(const string& fileId,const string& lastModified, size_t size);
 
private:
    string token;
};