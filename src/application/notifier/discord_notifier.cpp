#include "discord_notifier.hpp"
#include "../../infrastructure/logger.hpp"
#include <nlohmann/json.hpp>
#include <format>

namespace bluray::application::notifier {

using json = nlohmann::json;

DiscordNotifier::DiscordNotifier() {
    auto& config = infrastructure::ConfigManager::instance();
    webhook_url_ = config.get("discord_webhook_url", "");
}

void DiscordNotifier::notify(const domain::ChangeEvent& event) {
    if (!isConfigured()) {
        infrastructure::Logger::instance().warning("Discord notifier not configured");
        return;
    }

    // Build Discord webhook payload
    json payload = {
        {"content", buildMessage(event)},
        {"embeds", json::array({json::parse(buildEmbed(event))})}
    };

    // Send webhook
    auto response = client_.post(webhook_url_, payload.dump());

    if (response.success) {
        infrastructure::Logger::instance().info(
            std::format("Discord notification sent: {}", event.describe())
        );
    } else {
        infrastructure::Logger::instance().error(
            std::format("Failed to send Discord notification: {} (status: {})",
                       event.describe(), response.status_code)
        );
    }
}

bool DiscordNotifier::isConfigured() const {
    return !webhook_url_.empty();
}

std::string DiscordNotifier::buildMessage(const domain::ChangeEvent& event) const {
    switch (event.type) {
        case domain::ChangeType::PriceDroppedBelowThreshold:
            return std::format("🎉 **Price Alert!** - {}", event.item.title);

        case domain::ChangeType::BackInStock:
            return std::format("📦 **Back in Stock!** - {}", event.item.title);

        case domain::ChangeType::PriceChanged:
            return std::format("💰 Price Update - {}", event.item.title);

        case domain::ChangeType::OutOfStock:
            return std::format("⚠️ Out of Stock - {}", event.item.title);

        default:
            return std::format("ℹ️ Update - {}", event.item.title);
    }
}

std::string DiscordNotifier::buildEmbed(const domain::ChangeEvent& event) const {
    json embed = {
        {"title", event.item.title},
        {"url", event.item.url},
        {"color", 0x00ff00}, // Green
        {"timestamp", std::format("{:%Y-%m-%dT%H:%M:%S}",
                                  std::chrono::system_clock::now())},
        {"fields", json::array()}
    };

    // Set color based on event type
    switch (event.type) {
        case domain::ChangeType::PriceDroppedBelowThreshold:
            embed["color"] = 0x00ff00; // Green
            break;
        case domain::ChangeType::BackInStock:
            embed["color"] = 0x0099ff; // Blue
            break;
        case domain::ChangeType::OutOfStock:
            embed["color"] = 0xff0000; // Red
            break;
        default:
            embed["color"] = 0xffaa00; // Orange
            break;
    }

    // Add description
    embed["description"] = event.describe();

    // Add fields
    if (event.new_price) {
        embed["fields"].push_back({
            {"name", "Current Price"},
            {"value", std::format("€{:.2f}", *event.new_price)},
            {"inline", true}
        });
    }

    if (event.item.desired_max_price > 0) {
        embed["fields"].push_back({
            {"name", "Your Max Price"},
            {"value", std::format("€{:.2f}", event.item.desired_max_price)},
            {"inline", true}
        });
    }

    if (event.item.is_uhd_4k) {
        embed["fields"].push_back({
            {"name", "Format"},
            {"value", "🎬 UHD 4K"},
            {"inline", true}
        });
    }

    embed["fields"].push_back({
        {"name", "Source"},
        {"value", event.item.source},
        {"inline", true}
    });

    // Add thumbnail if available
    if (!event.item.image_url.empty()) {
        embed["thumbnail"] = {
            {"url", event.item.image_url}
        };
    }

    return embed.dump();
}

} // namespace bluray::application::notifier
