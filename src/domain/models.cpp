#include "models.hpp"
#include <format>

namespace bluray::domain {

std::string ChangeEvent::describe() const {
    switch (type) {
        case ChangeType::PriceDroppedBelowThreshold:
            return std::format(
                "Price dropped below threshold for '{}': €{:.2f} → €{:.2f} (threshold: €{:.2f})",
                item.title,
                old_price.value_or(0.0),
                new_price.value_or(0.0),
                item.desired_max_price
            );

        case ChangeType::BackInStock:
            return std::format(
                "'{}' is back in stock! Current price: €{:.2f}",
                item.title,
                item.current_price
            );

        case ChangeType::PriceChanged:
            return std::format(
                "Price changed for '{}': €{:.2f} → €{:.2f}",
                item.title,
                old_price.value_or(0.0),
                new_price.value_or(0.0)
            );

        case ChangeType::OutOfStock:
            return std::format(
                "'{}' is now out of stock",
                item.title
            );

        default:
            return "Unknown change";
    }
}

} // namespace bluray::domain
