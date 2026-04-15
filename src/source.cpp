#include "source.hpp"
#include <curl/curl.h>
#include <fileInfo.hpp>
#include "logger.hpp"
#include "retrylogic.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
using namespace std;

using json = nlohmann::json;
 
// ================= CALLBACKS =================
 
size_t writeCallback(void* contents, size_t size,
                     size_t nmemb, string* output)
{
    size_t total = size * nmemb;
    LOG_DEBUG("writeCallback received bytes: " + std::to_string(total));
 
    output->append((char*)contents, total);
    return total;
}
 
size_t fileWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t total = size * nmemb;
    DownloadContext* ctx = static_cast<DownloadContext*>(userdata);
    ctx->file->write((char*)ptr, total);
    ctx->totalDownloaded += total;
    //LOG_DEBUG("Downloaded: " + std::to_string(ctx->totalDownloaded) + " / " + std::to_string(ctx->expectedSize) + " bytes");
    return total;
}
 
source::source(const string& t) : token(t)
{
    LOG_INFO("Source initialized");
}

bool isAPIResponseValid(const json& j, const std::string& response)
{
   if (response.empty()) {
       LOG_ERROR("Empty response from API");
       return false;
   }
   if (j.contains("error")) {
       std::string code    = j["error"].value("code", "unknown");
       std::string message = j["error"].value("message", "unknown");
       LOG_ERROR("API error code: " + code);
       LOG_ERROR("API error message: " + message);
       if (code == "InvalidAuthenticationToken" ||
           code == "tokenExpired"              ||
           code == "unauthenticated") {
           LOG_ERROR("TOKEN IS INVALID OR EXPIRED — stopping execution");
       }
       return false;
   }
   if (!j.contains("value")) {
       LOG_ERROR("Unexpected response structure");
       LOG_ERROR("Raw response: " + response);
       return false;
   }
   return true;
}
 
std::string callAPI(const std::string& url, const std::string& token)
{
    LOG_INFO("Calling API: " + url);
 
    CURL* curl = curl_easy_init();
    std::string response;
 
    if (!curl) {
        LOG_ERROR("CURL init failed");
        return "";
    }
 
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers,
        ("Authorization: Bearer " + token).c_str());
 
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
    curl_easy_setopt(curl,CURLOPT_FOLLOWLOCATION, 1L);
 
    CURLcode res = curl_easy_perform(curl);
 
    if (res != CURLE_OK) {
        LOG_ERROR("CURL failed: " + std::string(curl_easy_strerror(res)));
    } else {
        LOG_INFO("API call successful, response size: " + std::to_string(response.size()));
    }
 
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
 
    return response;
}
 
// ================= LIST FILES =================
 
std::vector<fileInfo> source::listFilesRecursivefromOneDrive(std::string folderId)
{
    std::vector<fileInfo> files;
    std::string url;
 
    if (folderId == "root") {
        url = "https://graph.microsoft.com/v1.0/me/drive/root/children";
    } else {
        url = "https://graph.microsoft.com/v1.0/me/drive/items/" + folderId + "/children";
    }
 
    std::string response = callAPI(url, token);
    json j;
 
    try {
        j = json::parse(response);
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse failed: " + std::string(e.what()));
        return files;
    }

    if (!isAPIResponseValid(j, response)) {
       LOG_ERROR("Stopping — API response invalid, check token");
       return files;   // returns empty — BackupEngine will process 0 files
   }
   LOG_INFO("API response valid, processing files");
 
    if (!j.contains("value")) {
        LOG_ERROR("Invalid API response structure");
        LOG_ERROR("Raw API response :" + response );
        return files;
    }
 
    for (auto& item : j["value"]) {
 
        if (item.contains("folder")) {
 
            std::string folderName = item["name"];
            string folderId = item["id"];
            LOG_INFO("Entering folder: '" + folderName + "' (ID: " + folderId + ")");
            auto subFiles = listFilesRecursivefromOneDrive(item["id"]);
            LOG_INFO("Files inside '" + folderName +
                     "' = " + std::to_string(subFiles.size()));
 
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        }
        else {
            fileInfo f;
            f.id = item["id"];
            f.name = item["name"];
            f.size = item.value("size", 0);
            f.lastModified = item.value("lastModifiedDateTime", "");

            if (item.contains("@microsoft.graph.downloadUrl")) {
            f.downloadURL = item["@microsoft.graph.downloadUrl"];
            }
            else {//USE GRAPH CONTENT API (FIX)
                f.downloadURL = "https://graph.microsoft.com/v1.0/me/drive/items/" + std::string(item["id"]) + "/content";
            }
 
            LOG_INFO("File found: " + f.name +
                     " | Size: " + std::to_string(f.size));
 
            files.push_back(f);
        }
    }
    return files;
}
 
// ================= DOWNLOAD =================
bool source::downloadFile(const std::string& downloadUrl,
                         std::string& localPath,
                         size_t expectedSize)
{
    LOG_INFO("Starting download: " + localPath);
    return retryOperation([&]() {
 
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERROR("CURL init failed");
            return true;
        }
 
        std::ofstream file(localPath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open file: " + localPath);
            curl_easy_cleanup(curl);
            return true;
        }
        DownloadContext ctx;
        ctx.file = &file;
        ctx.expectedSize = expectedSize;
 
        LOG_DEBUG("Downloading from URL: " + downloadUrl);
        curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fileWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
 
        // Important stability options
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
 
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60L);
 
        CURLcode res = curl_easy_perform(curl);
 
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
 
        file.close();
        curl_easy_cleanup(curl);
 
        LOG_INFO("HTTP Code: " + std::to_string(http_code) +
                 " | CURL: " + curl_easy_strerror(res));
 
        // Verify size
        std::ifstream check(localPath, std::ios::binary | std::ios::ate);
        size_t actualSize = check.tellg();
 
        LOG_INFO("Downloaded size: " + std::to_string(actualSize) +
                 " Expected: " + std::to_string(expectedSize));
 
        if (res == CURLE_OK && http_code == 200 && actualSize == expectedSize) {
            LOG_INFO("Download successful: " + localPath);
            return true;
        }
 
        LOG_WARN("Retrying download...");
        return false;
 
    }, NO_OF_RETRIES);
}