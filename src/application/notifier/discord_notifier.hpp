#pragma once

#include "notifier.hpp"
#include "../../infrastructure/network_client.hpp"
#include "../../infrastructure/config_manager.hpp"

namespace bluray::application::notifier {

/**
 * Discord webhook notifier
 */
class DiscordNotifier : public INotifier {
public:
    DiscordNotifier();

    void notify(const domain::ChangeEvent& event) override;
    bool isConfigured() const override;

private:
    std::string buildMessage(const domain::ChangeEvent& event) const;
    std::string buildEmbed(const domain::ChangeEvent& event) const;

    infrastructure::NetworkClient client_;
    std::string webhook_url_;
};

} // namespace bluray::application::notifier
