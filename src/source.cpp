#include "source.hpp"
#include <fileInfo.hpp>
#include "logger.hpp"
#include "retrylogic.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <curl/curl.h>
using namespace std;

using json = nlohmann::json;
size_t writeCallback(void* contents, size_t size,
                     size_t nmemb, string* output)
{
    size_t total = size * nmemb;
    LOG_DEBUG("writeCallback received bytes: " + to_string(total));
 
    output->append((char*)contents, total);
    return total;
}
 
size_t fileWriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    size_t total = size * nmemb;
    DownloadContext* ctx = static_cast<DownloadContext*>(userdata);
    ctx->file->write((char*)ptr, total);
    ctx->totalDownloaded += total;
    return total;
}
 
source::source(const string& t) : token(t)
{
    LOG_INFO("Source initialized");
}

bool isAPIResponseValid(const json& j, const string& response)
{
   if (response.empty()) {
       LOG_ERROR("Empty response from API");
       return false;
   }
   if (j.contains("error")) {
       string code    = j["error"].value("code", "unknown");
       string message = j["error"].value("message", "unknown");
       LOG_ERROR("API error code: " + code);
       LOG_ERROR("API error message: " + message);
       if (code == "InvalidAuthenticationToken" || code == "tokenExpired" || code == "unauthenticated") {
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
 
string callAPI(const string& url, const string& token)
{
    LOG_INFO("Calling API: " + url);
    CURL* curl = curl_easy_init();
    string response;
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
        LOG_ERROR("CURL failed: " + string(curl_easy_strerror(res)));
    } else {
        LOG_INFO("API call successful, response size: " + to_string(response.size()));
    }
 
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return response;
}
 
vector<fileInfo> source::listFilesRecursivefromOneDrive(string folderId,  string parentPath)
{
    vector<fileInfo> files;
    string url;
    if (folderId == "root") {
        url = "https://graph.microsoft.com/v1.0/me/drive/root/children";
    } else {
        url = "https://graph.microsoft.com/v1.0/me/drive/items/" + folderId + "/children";
    }
 
    string response = callAPI(url, token);
    json j;
 
    try {
        j = json::parse(response);
    } catch (const exception& e) {
        LOG_ERROR("JSON parse failed: " + string(e.what()));
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
            string folderName = item["name"];
            string folderId = item["id"];
            string subPath = parentPath.empty()? folderName : parentPath + "/" + folderName;

            LOG_INFO("Folder: " + subPath + " | ID: " + folderId);

            auto subFiles = listFilesRecursivefromOneDrive(folderId, subPath);
            LOG_INFO("Files inside '" + folderName + "' = " + to_string(subFiles.size()));
            files.insert(files.end(), subFiles.begin(), subFiles.end());
        }
        else 
        {
            fileInfo f;
            f.id           = item["id"];
            f.name         = item["name"];
            f.folderPath   = parentPath;
            f.size         = item.value("size", 0);
            f.lastModified = item.value("lastModifiedDateTime", "");

            if (item.contains("@microsoft.graph.downloadUrl")) {
            f.downloadURL = item["@microsoft.graph.downloadUrl"];
            }
            else {
                f.downloadURL = "https://graph.microsoft.com/v1.0/me/drive/items/" + string(item["id"]) + "/content";
            }
            LOG_INFO("File found: " + f.folderPath + "/" + f.name + " | Size: " + to_string(f.size));
            files.push_back(f);
        }
    }
    return files;
}

bool source::downloadFile(const string& downloadUrl, string& localPath, size_t expectedSize)
{
    LOG_INFO("Starting download: " + localPath);
    return retryOperation([&]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERROR("CURL init failed");
            return true;
        }
 
        ofstream file(localPath, ios::binary);
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
        LOG_INFO("HTTP Code: " + to_string(http_code) + " | CURL: " + curl_easy_strerror(res));
 
        // Verify size
        ifstream check(localPath, ios::binary | ios::ate);
        size_t actualSize = check.tellg();
        LOG_INFO("Downloaded size: " + to_string(actualSize) + " Expected: " + to_string(expectedSize));
 
        if (res == CURLE_OK && http_code == 200 && actualSize == expectedSize) {
            LOG_INFO("Download successful: " + localPath);
            return true;
        }
        LOG_WARN("Retrying download...");
        return false;
    }, NO_OF_RETRIES);
}

bool source::verifyFileUnchanged(const string& fileId,const string& lastModified, size_t size)
{
   LOG_INFO("Verifying file unchanged after download: " + fileId);
   string url = "https://graph.microsoft.com/v1.0/me/drive/items/" + fileId + "?$select=id,name,size,lastModifiedDateTime";
   string response = callAPI(url, token);
   if (response.empty()) {
       LOG_WARN("Could not re-verify file metadata — proceeding with caution");
       return true;
   }
   json j;
   try {
       j = json::parse(response);
   } catch (const exception& e) {
       LOG_WARN("JSON parse failed during re-verify: " + string(e.what()));
       return true;
   }
   if (j.contains("error")) {
       LOG_WARN("API error during re-verify — proceeding: " + j["error"].value("message", "unknown"));
       return true;
   }
   string freshModified = j.value("lastModifiedDateTime", "");
   size_t freshSize     = j.value("size", (size_t)0);
   if (freshModified != lastModified || freshSize != size) {
       LOG_WARN("File changed during backup: " + fileId);
       LOG_WARN("Earlier  → lastModified=" + lastModified +" size=" + to_string(size));
       LOG_WARN("latest  → lastModified=" + freshModified +" size=" + to_string(freshSize));
       LOG_WARN("Skipping — will be picked up correctly next run");
       return false;
   }
   LOG_INFO("File verified unchanged: " + fileId);
   return true;
}