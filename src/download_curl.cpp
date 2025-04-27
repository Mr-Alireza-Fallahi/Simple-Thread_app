#include <curl/curl.h>
#include <download_curl.h>

size_t CurlDownloader::write_data(void* ptr, size_t size, size_t nmemb, void* userdata) {
    ThreadContext* ctx = static_cast<ThreadContext*>(userdata);
    std::ofstream out(ctx->temp_file, std::ios::binary | std::ios::app);
    out.write(static_cast<const char*>(ptr), size * nmemb);
    return out ? size * nmemb : 0;
}

CURL* CurlDownloader::create_curl_handle(ThreadContext* ctx) {
    CURL* curl = curl_easy_init();
    if(!curl) throw std::runtime_error("Failed to initialize CURL");
    
    curl_easy_setopt(curl, CURLOPT_URL, ctx->url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, ctx);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    return curl;
}

void CurlDownloader::download_segment(ThreadContext* ctx) {
    try {
        CURL* curl = create_curl_handle(ctx);
        const std::string range = std::to_string(ctx->start_range) + "-" + 
                                (ctx->end_range ? std::to_string(ctx->end_range) : "");
        curl_easy_setopt(curl, CURLOPT_RANGE, range.c_str());

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            throw std::runtime_error(curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    } catch(...) {
        ctx->error = std::current_exception();
    }
}

curl_off_t CurlDownloader::get_file_info(const std::string& url) {
    CURL* curl = curl_easy_init();
    if(!curl) throw std::runtime_error("CURL initialization failed");

    // First get file size with HEAD request
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  // HEAD method
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        curl_easy_cleanup(curl);
        throw std::runtime_error("HEAD request failed: " + std::string(curl_easy_strerror(res)));
    }

    // Get content length
    curl_off_t file_size;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &file_size);

    // Now test range support with actual GET request
    curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);  // Switch to GET method
    curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");  // Request first byte

    res = curl_easy_perform(curl);
    range_supported = (res == CURLE_OK);

    // Additional check for partial content response code
    if(range_supported) {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        range_supported = (response_code == 206);
    }

    curl_easy_cleanup(curl);
    return file_size;
}

FileSystem::path CurlDownloader::download(const std::string& url) {
    ThreadContext ctx{url, "", 0, 0, FileSystem::temp_directory_path() / "dl_single", nullptr};

    CURL* curl = create_curl_handle(&ctx);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if(res != CURLE_OK) {
        FileSystem::remove(ctx.temp_file);
        throw std::runtime_error(curl_easy_strerror(res));
    }

    const FileSystem::path final_path = FileSystem::path(url).filename();
    FileSystem::rename(ctx.temp_file, final_path);
    return final_path;
}

FileSystem::path CurlDownloader::download(const std::string& url, std::vector<std::thread>& threads) {
    const size_t num_threads = threads.capacity();
    threads.clear();
    
    // Get file information
    const curl_off_t file_size = get_file_info(url);
    if(file_size <= 0 || !range_supported) {
        std::lock_guard<std::mutex> lock(console_mutex);
        std::cout << "Falling back to single-threaded download\n";
        return download(url);
    }

    // Prepare download context
    const FileSystem::path output_path = FileSystem::path(url).filename();
    std::vector<ThreadContext> contexts(num_threads);
    std::vector<FileSystem::path> temp_files;

    // Calculate ranges and create temp files
    const curl_off_t chunk_size = file_size / num_threads;
    for(size_t i = 0; i < num_threads; ++i) {
        contexts[i].url = url;
        contexts[i].output_path = output_path;
        contexts[i].start_range = i * chunk_size;
        contexts[i].end_range = (i == num_threads-1) ? file_size-1 : (i+1)*chunk_size-1;
        contexts[i].temp_file = FileSystem::temp_directory_path() / ("part_" + std::to_string(i));
        temp_files.push_back(contexts[i].temp_file);
    }

    // Launch threads
    for(auto& ctx : contexts) {
        threads.emplace_back(&CurlDownloader::download_segment, this, &ctx);
    }

    // Wait for completion (caller must join threads)
    // This lambda can be called after joining threads
    auto finalize = [this, temp_files, output_path, &contexts]() {
        // Check for errors
        for(const auto& ctx : contexts) {
            if(ctx.error) {
                for(const auto& f : temp_files) FileSystem::remove(f);
                std::rethrow_exception(ctx.error);
            }
        }
        
        // Merge files
        merge_files(temp_files, output_path);
        return output_path;
    };

    // Return a future or handle to finalize (simplified here)
    std::cout << "Download started with " << num_threads << " threads\n";
    return output_path; // Actual file will be ready after threads finish + finalize
}