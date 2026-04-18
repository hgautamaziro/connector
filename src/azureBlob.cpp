#include "azureBlob.hpp"
#include "retrylogic.hpp"
#include "logger.hpp"
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>
using namespace std;

//baseURL -> main Storage part
//sastoken ->Authentication part 
azureBlob::azureBlob(const string& sasURL) {
 
    LOG_INFO("Initializing Azure Blob client");
    size_t pos = sasURL.find('?');
    if (pos == string::npos) {
        LOG_ERROR("Invalid SAS URL provided");
        throw invalid_argument("Invalid SAS URL");
    }
    baseUrl = sasURL.substr(0, pos);
    sasToken = sasURL.substr(pos);
    LOG_DEBUG("Base URL parsed: " + baseUrl);
}

UploadStatus azureBlob::uploadInBlob(const string& filename,const string& localPath)
{
    LOG_INFO("Starting upload for file: " + filename);
    string uploadURL = baseUrl + "/" + filename + sasToken;
    UploadStatus finalStatus = UploadStatus::SERVER_ERROR;
    bool success = retryOperation([&]() {
        LOG_DEBUG("Preparing CURL request");
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERROR("CURL init failed");
            finalStatus = UploadStatus::NETWORK_ERROR;
            return true;
        }
 
        FILE* file = fopen(localPath.c_str(), "rb");
        if (!file) {
            LOG_ERROR("Failed to open file: " + localPath);
            curl_easy_cleanup(curl);
            finalStatus = UploadStatus::CLIENT_ERROR;
            return true;
        }
 
        LOG_DEBUG("File opened successfully");
 
        fseek(file, 0, SEEK_END);
        curl_off_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        LOG_DEBUG("File size: " + std::to_string(fileSize));
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "x-ms-blob-type: BlockBlob");
        headers = curl_slist_append(headers, "Expect:");
 
        curl_easy_setopt(curl, CURLOPT_URL, uploadURL.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_READDATA, file);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, fileSize);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        LOG_DEBUG("Executing CURL request");
        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        LOG_INFO("HTTP Code: " + to_string(http_code) +" | CURL: " + curl_easy_strerror(res));
 
        fclose(file);
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
 
        //Success
        if (res == CURLE_OK && (http_code == 200 || http_code == 201)) {
            LOG_INFO("Upload successful: " + filename);
            finalStatus = UploadStatus::SUCCESS;
            return true;
        }
 
        //Client error
        if (http_code >= 400 && http_code < 500) {
            LOG_WARN("Client error occurred. HTTP Code: " +to_string(http_code));
 
            if (http_code == 403)
                finalStatus = UploadStatus::AUTH_ERROR;
            else
                finalStatus = UploadStatus::CLIENT_ERROR;
 
            return true;
        }
        // Retry case
        LOG_WARN("Retrying upload due to server/network issue");
        return false;
    }, NO_OF_RETRIES);
 
    if (!success) {
        LOG_ERROR("Upload failed after retries for file: " + filename);
    }
    return finalStatus;
}