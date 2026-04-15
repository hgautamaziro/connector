#pragma once
#include <unordered_map>
#include <unordered_set>
#include <cstddef>
#include <string>
using namespace std;

struct backUp{
	string lastModified;
	string originalName;
	string dupName;
	size_t size;
};


//load and save operates on the local metadata file that track the previously backedup files
// Incremental back up by comparing the last modified timestamp
class metaDataStore{

	public:
	bool loadFile(const string& file); // read the local json file and update the record_data map 
	bool saveFile(const string& file);
	bool isModified(const string& id, const string& lastModified, size_t size);
	void update(const string& id,const string& lastModified, size_t size,const string& originalName, const string& dupName);
	
	//return all blob names already used in the previous session
	unordered_set<string> getUsedBlobNames() const;
	
	//returns the existing blob name for File ID 
	string getExistingBlobName(const string& id) const;

	private:
	std::unordered_map<std::string, backUp> record_data;
	//  key(file ID)(string )   value (backup)
	//
	//   file1                  {lastModified,size}
	//   file2                  {lastModified, size}
	
};