#include <instance.h>

std::shared_ptr<IDownloader> create_instance(DownloaderFramework framework)
{
    switch (framework)
    {
    case DownloaderFramework::CURL:
        return std::make_shared<CurlDownloader>();
        break;
    
    default:
        break;
    }
}
