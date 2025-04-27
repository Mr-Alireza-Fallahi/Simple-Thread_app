#include <download_curl.h>
#include <download_interface.h>

#include <memory>

enum class DownloaderFramework {
    CURL
};

// Factory Method for downloader class
std::shared_ptr<IDownloader> create_instance(DownloaderFramework framework);
