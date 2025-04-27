#include <download_curl.h>

size_t CurlDownloader::write_callback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

std::string CurlDownloader::extract_filename(const char* url) const {
    std::string s(url);
        
    // Remove query parameters and fragments
    s.erase(std::find(s.begin(), s.end(), '?'), s.end());
    s.erase(std::find(s.begin(), s.end(), '#'), s.end());
    
    // Extract filename from URL
    size_t last_slash = s.find_last_of('/');
    if(last_slash == std::string::npos || last_slash + 1 == s.length()) {
        return "downloaded_file";
    }
    
    std::string filename = s.substr(last_slash + 1);
    return filename.empty() ? "downloaded_file" : filename;
}

std::filesystem::path CurlDownloader::download(const std::string& url) {
    CURL* curl = curl_easy_init();
        if(!curl) throw std::runtime_error("Failed to initialize CURL");

        // Generate temporary filename
        const std::string temp_file = std::tmpnam(nullptr);
        FILE* fp = fopen(temp_file.c_str(), "wb");
        if(!fp) {
            curl_easy_cleanup(curl);
            throw std::runtime_error("Failed to create temporary file");
        }

        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

        // Perform download
        CURLcode res = curl_easy_perform(curl);
        fclose(fp);

        // Handle errors
        if(res != CURLE_OK) {
            std::filesystem::remove(temp_file);
            curl_easy_cleanup(curl);
            throw std::runtime_error(curl_easy_strerror(res));
        }

        // Get final URL after redirects
        char* effective_url = nullptr;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url);
        
        // Determine final filename
        std::string filename = extract_filename(effective_url);
        std::filesystem::path target_path = filename;

        // Rename temporary file to final name
        try {
            std::filesystem::rename(temp_file, target_path);
        } catch(...) {
            // If rename fails (e.g., file exists), use temporary name
            target_path = temp_file;
        }

        curl_easy_cleanup(curl);
        return target_path;
}