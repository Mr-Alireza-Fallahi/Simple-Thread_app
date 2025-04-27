#include <download_interface.h>

#include <curl/curl.h>
#include <stdexcept>
#include <mutex>
// #include <algorithm>
#include <fstream>
#include <iostream>

using std::size_t;

struct ThreadContext {
    std::string url;
    FileSystem::path output_path;
    curl_off_t start_range;
    curl_off_t end_range;
    FileSystem::path temp_file;
    std::exception_ptr error;
};

// Curl implementation

class CurlDownloader : public IDownloader {
private:
    std::mutex console_mutex;
    bool range_supported;

    // Callback for writing data to temp files
    static size_t write_data(void* ptr, size_t size, size_t nmemb, void* userdata);

    // Common initialization for curl handles
    CURL* create_curl_handle(ThreadContext* ctx);

    // Thread worker function
    void download_segment(ThreadContext* ctx);

    // Get file size and check range support
    curl_off_t get_file_info(const std::string& url);

    // Merge temporary files into final output
    void merge_files(const std::vector<FileSystem::path>& temp_files, const FileSystem::path& output);

public:

    CurlDownloader() : range_supported(false) {}
    // Single-threaded implementation
    FileSystem::path download(const std::string& url) override;

    // Multi-threaded implementation
    FileSystem::path download(const std::string& url, std::vector<std::thread>& threads) override;
};