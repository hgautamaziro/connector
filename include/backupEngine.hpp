#pragma once
#include "metaDataStore.hpp"
#include "source.hpp"
#include "azureBlob.hpp"
#define NO_OF_RETRIES    3


class BackupEngine {
    public:
    BackupEngine(const std::string& token, const std::string& sasUrl);
    void start();
    void testUpload();

    private:
    source src;
    azureBlob dest;
    metaDataStore meta_data_record; 
};