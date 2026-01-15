#include "network_client.hpp"
#include "logger.hpp"
#include <format>

namespace bluray::infrastructure {

NetworkClient::NetworkClient() {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("Failed to initialize libcurl");
    }
    setupCommonOptions();
}

NetworkClient::~NetworkClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
    }
}

NetworkClient::NetworkClient(NetworkClient&& other) noexcept
    : curl_(other.curl_),
      user_agent_(std::move(other.user_agent_)),
      timeout_(other.timeout_),
      follow_redirects_(other.follow_redirects_) {
    other.curl_ = nullptr;
}

NetworkClient& NetworkClient::operator=(NetworkClient&& other) noexcept {
    if (this != &other) {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_ = other.curl_;
        user_agent_ = std::move(other.user_agent_);
        timeout_ = other.timeout_;
        follow_redirects_ = other.follow_redirects_;
        other.curl_ = nullptr;
    }
    return *this;
}

HttpResponse NetworkClient::get(
    std::string_view url,
    const std::vector<std::string>& headers
) {
    HttpResponse response;
    std::string response_body;

    curl_easy_setopt(curl_, CURLOPT_URL, std::string(url).c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);

    // Set headers
    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    if (header_list) {
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);
    }

    // Perform request
    const CURLcode res = curl_easy_perform(curl_);

    // Cleanup headers
    if (header_list) {
        curl_slist_free_all(header_list);
    }

    if (res != CURLE_OK) {
        Logger::instance().error(
            std::format("GET request failed for {}: {}", url, curl_easy_strerror(res))
        );
        return response;
    }

    long status_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);

    char* content_type = nullptr;
    curl_easy_getinfo(curl_, CURLINFO_CONTENT_TYPE, &content_type);

    response.status_code = static_cast<int>(status_code);
    response.body = std::move(response_body);
    response.content_type = content_type ? content_type : "";
    response.success = (status_code >= 200 && status_code < 300);

    return response;
}

HttpResponse NetworkClient::post(
    std::string_view url,
    std::string_view json_body,
    const std::vector<std::string>& headers
) {
    HttpResponse response;
    std::string response_body;

    curl_easy_setopt(curl_, CURLOPT_URL, std::string(url).c_str());
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, std::string(json_body).c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);

    // Set headers
    struct curl_slist* header_list = nullptr;
    for (const auto& header : headers) {
        header_list = curl_slist_append(header_list, header.c_str());
    }
    // Add default Content-Type for JSON if not provided
    bool has_content_type = false;
    for (const auto& header : headers) {
        if (header.find("Content-Type") != std::string::npos) {
            has_content_type = true;
            break;
        }
    }
    if (!has_content_type) {
        header_list = curl_slist_append(header_list, "Content-Type: application/json");
    }
    if (header_list) {
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, header_list);
    }

    // Perform request
    const CURLcode res = curl_easy_perform(curl_);

    // Cleanup headers
    if (header_list) {
        curl_slist_free_all(header_list);
    }

    if (res != CURLE_OK) {
        Logger::instance().error(
            std::format("POST request failed for {}: {}", url, curl_easy_strerror(res))
        );
        return response;
    }

    long status_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);

    char* content_type = nullptr;
    curl_easy_getinfo(curl_, CURLINFO_CONTENT_TYPE, &content_type);

    response.status_code = static_cast<int>(status_code);
    response.body = std::move(response_body);
    response.content_type = content_type ? content_type : "";
    response.success = (status_code >= 200 && status_code < 300);

    return response;
}

std::optional<std::vector<uint8_t>> NetworkClient::downloadFile(
    std::string_view url
) {
    std::vector<uint8_t> file_data;

    curl_easy_setopt(curl_, CURLOPT_URL, std::string(url).c_str());
    curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &file_data);

    const CURLcode res = curl_easy_perform(curl_);

    if (res != CURLE_OK) {
        Logger::instance().error(
            std::format("File download failed for {}: {}", url, curl_easy_strerror(res))
        );
        return std::nullopt;
    }

    long status_code = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &status_code);

    if (status_code < 200 || status_code >= 300) {
        Logger::instance().error(
            std::format("File download failed with status {}: {}", status_code, url)
        );
        return std::nullopt;
    }

    return file_data;
}

void NetworkClient::setUserAgent(std::string_view user_agent) {
    user_agent_ = user_agent;
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, user_agent_.c_str());
}

void NetworkClient::setTimeout(long timeout_seconds) {
    timeout_ = timeout_seconds;
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout_);
}

void NetworkClient::setFollowRedirects(bool follow) {
    follow_redirects_ = follow;
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, follow ? 1L : 0L);
}

size_t NetworkClient::writeCallback(
    void* contents,
    size_t size,
    size_t nmemb,
    void* userp
) {
    const size_t total_size = size * nmemb;

    if (auto* str = static_cast<std::string*>(userp)) {
        str->append(static_cast<char*>(contents), total_size);
    } else if (auto* vec = static_cast<std::vector<uint8_t>*>(userp)) {
        const auto* bytes = static_cast<uint8_t*>(contents);
        vec->insert(vec->end(), bytes, bytes + total_size);
    }

    return total_size;
}

void NetworkClient::setupCommonOptions() {
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, user_agent_.c_str());
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeout_);
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, follow_redirects_ ? 1L : 0L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 2L);
}

} // namespace bluray::infrastructure
