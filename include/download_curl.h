#include <download_interface.h>

#include <curl/curl.h>
#include <stdexcept>
#include <algorithm>

using std::size_t;

// Curl implementation
class CurlDownloader : public IDownloader {
private:
    static size_t write_callback(void* ptr, size_t size, size_t nmemb, FILE* stream);
    std::string extract_filename(const char* url) const;
public:
    CurlDownloader() = default;
    ~CurlDownloader() = default;
    std::filesystem::path download(const std::string& url) override;
};