#include "email_notifier.hpp"
#include "../../infrastructure/logger.hpp"
#include <format>
#include <sstream>

namespace bluray::application::notifier {

EmailNotifier::EmailNotifier() {
    auto& config_mgr = infrastructure::ConfigManager::instance();

    config_.smtp_server = config_mgr.get("smtp_server", "");
    config_.smtp_port = config_mgr.getInt("smtp_port", 587);
    config_.smtp_user = config_mgr.get("smtp_user", "");
    config_.smtp_pass = config_mgr.get("smtp_pass", "");
    config_.from_address = config_mgr.get("smtp_from", "");
    config_.to_address = config_mgr.get("smtp_to", "");
}

EmailNotifier::~EmailNotifier() = default;

void EmailNotifier::notify(const domain::ChangeEvent& event) {
    if (!isConfigured()) {
        infrastructure::Logger::instance().warning("Email notifier not configured");
        return;
    }

    const std::string subject = buildSubject(event);
    const std::string body = buildEmailBody(event);

    if (sendEmail(subject, body)) {
        infrastructure::Logger::instance().info(
            std::format("Email notification sent: {}", event.describe())
        );
    } else {
        infrastructure::Logger::instance().error(
            std::format("Failed to send email notification: {}", event.describe())
        );
    }
}

bool EmailNotifier::isConfigured() const {
    return !config_.smtp_server.empty() &&
           !config_.smtp_user.empty() &&
           !config_.from_address.empty() &&
           !config_.to_address.empty();
}

std::string EmailNotifier::buildSubject(const domain::ChangeEvent& event) const {
    switch (event.type) {
        case domain::ChangeType::PriceDroppedBelowThreshold:
            return std::format("Price Alert: {} - €{:.2f}",
                             event.item.title, event.new_price.value_or(0.0));

        case domain::ChangeType::BackInStock:
            return std::format("Back in Stock: {}", event.item.title);

        case domain::ChangeType::PriceChanged:
            return std::format("Price Update: {}", event.item.title);

        case domain::ChangeType::OutOfStock:
            return std::format("Out of Stock: {}", event.item.title);

        default:
            return std::format("Blu-ray Tracker Update: {}", event.item.title);
    }
}

std::string EmailNotifier::buildEmailBody(const domain::ChangeEvent& event) const {
    std::ostringstream oss;

    oss << "Blu-ray Tracker Notification\n";
    oss << "============================\n\n";

    oss << event.describe() << "\n\n";

    oss << "Product Details:\n";
    oss << "---------------\n";
    oss << "Title: " << event.item.title << "\n";
    oss << "URL: " << event.item.url << "\n";
    oss << "Source: " << event.item.source << "\n";

    if (event.new_price) {
        oss << "Current Price: €" << std::format("{:.2f}", *event.new_price) << "\n";
    }

    if (event.old_price && event.old_price != event.new_price) {
        oss << "Previous Price: €" << std::format("{:.2f}", *event.old_price) << "\n";
    }

    if (event.item.desired_max_price > 0) {
        oss << "Your Max Price: €" << std::format("{:.2f}", event.item.desired_max_price) << "\n";
    }

    if (event.item.is_uhd_4k) {
        oss << "Format: UHD 4K\n";
    }

    oss << "Stock Status: " << (event.item.in_stock ? "In Stock" : "Out of Stock") << "\n";

    oss << "\n--\n";
    oss << "Blu-ray Tracker\n";

    return oss.str();
}

bool EmailNotifier::sendEmail(const std::string& subject, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }

    // Build email payload
    std::ostringstream payload_stream;
    payload_stream << "To: " << config_.to_address << "\r\n";
    payload_stream << "From: " << config_.from_address << "\r\n";
    payload_stream << "Subject: " << subject << "\r\n";
    payload_stream << "Content-Type: text/plain; charset=UTF-8\r\n";
    payload_stream << "\r\n";
    payload_stream << body;

    EmailPayload payload;
    payload.data = payload_stream.str();

    // Set URL (smtp://server:port)
    const std::string url = std::format("smtp://{}:{}", config_.smtp_server, config_.smtp_port);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    // TLS settings
    curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Authentication
    curl_easy_setopt(curl, CURLOPT_USERNAME, config_.smtp_user.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config_.smtp_pass.c_str());

    // Email addresses
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, config_.from_address.c_str());

    struct curl_slist* recipients = nullptr;
    recipients = curl_slist_append(recipients, config_.to_address.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    // Payload
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payloadSource);
    curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    // Timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // Perform
    const CURLcode res = curl_easy_perform(curl);

    // Cleanup
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        infrastructure::Logger::instance().error(
            std::format("SMTP failed: {}", curl_easy_strerror(res))
        );
        return false;
    }

    return true;
}

size_t EmailNotifier::payloadSource(void* ptr, size_t size, size_t nmemb, void* userp) {
    auto* payload = static_cast<EmailPayload*>(userp);

    const size_t total_size = size * nmemb;
    const size_t remaining = payload->data.size() - payload->bytes_read;
    const size_t to_copy = std::min(total_size, remaining);

    if (to_copy > 0) {
        std::memcpy(ptr, payload->data.data() + payload->bytes_read, to_copy);
        payload->bytes_read += to_copy;
    }

    return to_copy;
}

} // namespace bluray::application::notifier
