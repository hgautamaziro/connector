
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include "fileInfo.hpp"
#include "metaDataStore.hpp"
#include "logger.hpp"

using namespace std;
using json = nlohmann::json;
 
bool metaDataStore::loadFile(const std::string& file)
{
    LOG_INFO("Loading metadata file: " + file);
    ifstream in_file(file);
 
    if (!in_file) {
        LOG_ERROR("Failed to open file: " + file);
        return false;
    }
    LOG_DEBUG("File opened successfully");
    json json_file;
 
    try {
        in_file >> json_file;
    } catch (const std::exception& e) {
        LOG_ERROR("JSON parse failed: " + std::string(e.what()));
        return false;
    }
 
    LOG_INFO("Parsing JSON entries");
    for (auto& [id, val] : json_file.items()) {
        LOG_DEBUG("Processing ID: " + id);
        backUp rec;
        rec.lastModified = val["lastModified"].get<std::string>();
        rec.size = val["size"].get<size_t>();
        rec.originalName = val.value("originalName", "");
        rec.dupName = val.value("dupName", "");
 
        record_data[id] = rec;
        LOG_DEBUG("Stored record -> lastModified: " +
          val["lastModified"].get<string>() +
          ", size: " + std::to_string(val["size"].get<size_t>()) +
          ", originalName: " +
          val.value("originalName", "") +
          ", dupName: " +
          val.value("dupName", ""));
    }
 
    LOG_INFO("Metadata loaded successfully");
    return true;
}
 
bool metaDataStore::saveFile(const std::string& file)
{
    try {
        std::ofstream out_source(file);
        if (!out_source.is_open()) {
            LOG_ERROR("Cannot open file for writing: " + file);
            return false;
        }
        json json_file;
 
        for (auto& [id, rec] : record_data) {
 
            LOG_DEBUG("Writing record : " + rec.originalName + "(blob: " + rec.dupName + ")");
            json_file[id] = {
                {"lastModified", rec.lastModified},
                {"size", rec.size},
                {"originalName", rec.originalName},
                {"dupName",rec.dupName}
            };
        }
 
        out_source << json_file.dump(4);
 
        if (!out_source.good()) {
            LOG_ERROR("Failed to write data to file: " + file);
            return false;
        }
 
        out_source.close();
 
        LOG_INFO("Metadata saved successfully");
 
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception while saving metadata: " + std::string(e.what()));
        return false;
    }
}
 
bool metaDataStore::isModified(const std::string& id,const std::string& lastModified,size_t size)
{ 
    if (record_data.find(id) == record_data.end()) {
        LOG_INFO("New file detected: " + id);
        return true;
    }
    bool modified = record_data[id].lastModified != lastModified || record_data[id].size != size;
    if (modified) {
        LOG_INFO("File modified: " + id);
    } else {
        LOG_DEBUG("File unchanged: " + id);
    }
    return modified;
}
 
void metaDataStore::update(const std::string& id,const std::string& lastModified,size_t size, const std::string& originalName,const std::string& dupName)
{
    LOG_INFO("Updating metadata for ID: " + id);
    backUp rec;
    rec.lastModified = lastModified;
    rec.size = size;
    rec.originalName = originalName;
    rec.dupName = dupName;
    record_data[id] = rec;
   
}

std::unordered_set<std::string> metaDataStore::getUsedBlobNames() const
{
   LOG_DEBUG("Fetching all used blob names from metadata");
   std::unordered_set<std::string> names;
   for (auto& [id, rec] : record_data) {
       if (!rec.dupName.empty()) {
           names.insert(rec.dupName);
           LOG_DEBUG("Found blob name: " + rec.dupName + " for ID: " + id);
        } else {
           LOG_DEBUG("Skipping ID: " + id + " (dupName is empty)");
        }
   }
   LOG_INFO("Total used blob names loaded: " + std::to_string(names.size()));
   return names;
}

std::string metaDataStore::getExistingBlobName(const std::string& id) const
{
   auto it = record_data.find(id);
   if (it != record_data.end()) {
       LOG_DEBUG("Found existing blob : " + it->second.dupName + " for file: " + it->second.originalName);
       return it->second.dupName;
   }
   LOG_DEBUG("No existing blob name found for ID: " + id);
   return "";
}