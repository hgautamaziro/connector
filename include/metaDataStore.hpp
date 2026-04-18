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

class metaDataStore{
	public:
	bool loadFile(const string& file);
	bool saveFile(const string& file);
	bool isModified(const string& id, const string& lastModified, size_t size);
	void update(const string& id,const string& lastModified, size_t size,const string& originalName, const string& dupName);
	unordered_set<string> getUsedBlobNames() const;      //return all blob names already used in the previous session
	string getExistingBlobName(const string& id) const;  //returns the existing blob name for File ID 

	private:
	std::unordered_map<std::string, backUp> record_data;
};