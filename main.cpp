#include <iostream>
#include <instance.h>

int main()
{
    std::shared_ptr<IDownloader> downloader = create_instance(DownloaderFramework::CURL);
    
    std::cout << "Test\n";
    return 0;
}