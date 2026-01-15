#pragma once

#include "notifier.hpp"
#include "../../infrastructure/config_manager.hpp"
#include <curl/curl.h>
#include <string>

namespace bluray::application::notifier {

/**
 * Email notifier via SMTP
 */
class EmailNotifier : public INotifier {
public:
    EmailNotifier();
    ~EmailNotifier();

    void notify(const domain::ChangeEvent& event) override;
    bool isConfigured() const override;

private:
    struct EmailConfig {
        std::string smtp_server;
        int smtp_port{587};
        std::string smtp_user;
        std::string smtp_pass;
        std::string from_address;
        std::string to_address;
    };

    struct EmailPayload {
        std::string data;
        size_t bytes_read{0};
    };

    static size_t payloadSource(void* ptr, size_t size, size_t nmemb, void* userp);

    std::string buildEmailBody(const domain::ChangeEvent& event) const;
    std::string buildSubject(const domain::ChangeEvent& event) const;
    bool sendEmail(const std::string& subject, const std::string& body);

    EmailConfig config_;
};

} // namespace bluray::application::notifier
