cmake_minimum_required(VERSION 3.10)
project(connector)

# C++ version  
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include header files folder
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("D:/Downloads/curl/include")

# Add executable with correct paths
#link_directories("D:/Downloads/curl/lib")

add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX)

# Source files
set(SOURCES
   main.cpp
   src/backupEngine.cpp
   src/source.cpp
   src/azureBlob.cpp
   src/metaDataStore.cpp
   src/retrylogic.cpp
   src/logger.cpp
   src/util.cpp
   src/config.cpp
)

find_package(CURL REQUIRED)
# Executable
add_executable(app ${SOURCES})
target_link_libraries(app CURL::libcurl)