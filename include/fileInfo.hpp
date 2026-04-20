#pragma once
#include <cstddef>
#include <string>


struct fileInfo{
	std::string id;
	std::string name;
	std::string folderPath;
	std::string lastModified;
	size_t size;
	std::string downloadURL;
};