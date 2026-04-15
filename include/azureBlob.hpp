#pragma once
#include <string>
#define NO_OF_RETRIES             3

enum class UploadStatus {
   SUCCESS,
   SKIPPED,
   AUTH_ERROR,
   CLIENT_ERROR,
   SERVER_ERROR,
   NETWORK_ERROR,
   FILE_MODIFIED,
   FILE_ERROR
};

enum class ConflictStrategy {
   OVERWRITE,  // replace existing blob
   SKIP,       // do nothing if blob exists
   RENAME      // auto-rename with timestamp (safest, default)
};
 
class azureBlob {
public:
    // azureBlob(const std::string& baseUrl, const std::string& sasToken)
    //    : baseUrl_(baseUrl), sasToken_(sasToken) {}
    azureBlob(const std::string& sasUrl);
    UploadStatus uploadInBlob(const std::string& name,
                const std::string& data);
 
private:
    std::string baseUrl;
    std::string sasToken;
};