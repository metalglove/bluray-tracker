#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <optional>
#include <curl/curl.h>

namespace bluray::infrastructure {

/**
 * HTTP response
 */
struct HttpResponse {
    int status_code{0};
    std::string body;
    std::string content_type;
    bool success{false};
};

/**
 * RAII wrapper for libcurl
 */
class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Prevent copying
    NetworkClient(const NetworkClient&) = delete;
    NetworkClient& operator=(const NetworkClient&) = delete;

    // Allow moving
    NetworkClient(NetworkClient&& other) noexcept;
    NetworkClient& operator=(NetworkClient&& other) noexcept;

    /**
     * Perform HTTP GET request
     */
    [[nodiscard]] HttpResponse get(
        std::string_view url,
        const std::vector<std::string>& headers = {}
    );

    /**
     * Perform HTTP POST request with JSON body
     */
    [[nodiscard]] HttpResponse post(
        std::string_view url,
        std::string_view json_body,
        const std::vector<std::string>& headers = {}
    );

    /**
     * Download file to memory
     */
    [[nodiscard]] std::optional<std::vector<uint8_t>> downloadFile(
        std::string_view url
    );

    /**
     * Set user agent
     */
    void setUserAgent(std::string_view user_agent);

    /**
     * Set timeout in seconds
     */
    void setTimeout(long timeout_seconds);

    /**
     * Set follow redirects
     */
    void setFollowRedirects(bool follow);

private:
    static size_t writeCallback(
        void* contents,
        size_t size,
        size_t nmemb,
        void* userp
    );

    void setupCommonOptions();

    CURL* curl_{nullptr};
    std::string user_agent_{
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/120.0.0.0 Safari/537.36"
    };
    long timeout_{30};
    bool follow_redirects_{true};
};

} // namespace bluray::infrastructure
