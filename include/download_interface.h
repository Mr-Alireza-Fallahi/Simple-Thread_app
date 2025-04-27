#pragma once 

// #include <string>
#include <vector>
#include <thread>
#include <filesystem>

namespace FileSystem = std::filesystem;

class IDownloader 
{
public:
    IDownloader() = default;
    ~IDownloader() = default;
    virtual FileSystem::path download(const std::string& url) = 0;
    virtual FileSystem::path download(const std::string& url, std::vector<std::thread>& threadList) = 0;
};