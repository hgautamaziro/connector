#include <iostream>
#include<unordered_set>
#include <string>
#include "backupEngine.hpp"
#include "fileInfo.hpp"
#include "azureBlob.hpp"
#include "logger.hpp"
#include "util.hpp"
using namespace std;

BackupEngine::BackupEngine(const string& token, const string& sasUrl)
    : src(token), dest(sasUrl) {}

void BackupEngine::start()
{
   LOG_INFO("BackupEngine started");
   meta_data_record.loadFile("metadata.json");
   
   unordered_set<string> usedNames = meta_data_record.getUsedBlobNames();
   LOG_INFO("Loaded " + to_string(usedNames.size()) + " existing blob names from metadata");

   LOG_INFO("Fetching files from OneDrive");
   auto files = src.listFilesRecursivefromOneDrive("root");
   if (files.empty()) {
       LOG_ERROR("No files returned from OneDrive — token may be invalid or expired");
       return;
   }
   for (auto& file : files){
       bool modified = meta_data_record.isModified(file.id, file.lastModified, file.size);
       if (!modified) {
           LOG_INFO("Skipping (not modified): " + file.name);
           continue;
        }
        LOG_INFO("Processing file: " + file.name);
        string localPath = "temp_" + file.name;
        LOG_DEBUG("Downloading: " + file.name + " | Size: " + std::to_string(file.size));
        if (!src.downloadFile(file.downloadURL, localPath, file.size)) {
           LOG_ERROR("Download failed: " + file.name);
           continue;
        }

        //Reuse the same blob name if this file was backed up before
        string existingBlob = meta_data_record.getExistingBlobName(file.id);
        string dupName ;
        if (!existingBlob.empty()) {  // File existed before..overwrite the same blob
           dupName = existingBlob;
           LOG_INFO("Re-uploading modified file : " + file.name +"to existing Blob: " + dupName);
        } 
        else {
           dupName = generateDuplicateName(file.name, usedNames);
           usedNames.insert(dupName);
           if (dupName != file.name) 
           {
               LOG_INFO("Duplicate name detected! '" + file.name +"' already exists -> renamed to: '" + dupName + "'");
           } 
           else 
           {
               LOG_INFO("New file, assigned blob name: " + dupName);
            }
       }
       UploadStatus status = dest.uploadInBlob(dupName, localPath);
       if (status == UploadStatus::SUCCESS) {
           meta_data_record.update(file.id, file.lastModified, file.size, file.name, dupName);
           LOG_INFO("Upload successful: " + file.name + " -> blob: " + dupName);
       }
       else if (status == UploadStatus::AUTH_ERROR) {
           LOG_ERROR("Upload failed (AUTH_ERROR): " + file.name);
           continue;
       }
       else if (status == UploadStatus::CLIENT_ERROR) {
           LOG_ERROR("Upload failed (CLIENT_ERROR): " + file.name);
           continue;
       }
       else {
           LOG_WARN("Upload failed (SERVER/NETWORK): " + file.name);
           continue;
       }
   }
   meta_data_record.saveFile("metadata.json");
   LOG_INFO("BackupEngine completed");
}