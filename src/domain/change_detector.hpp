#pragma once

#include "models.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace bluray::domain {

/**
 * Observer interface for change notifications
 */
class IChangeObserver {
public:
    virtual ~IChangeObserver() = default;
    virtual void onChangeDetected(const ChangeEvent& event) = 0;
};

/**
 * Subject class that detects changes and notifies observers (Observer pattern)
 */
class ChangeDetector {
public:
    /**
     * Register an observer to receive change notifications
     */
    void addObserver(std::shared_ptr<IChangeObserver> observer) {
        observers_.push_back(std::move(observer));
    }

    /**
     * Remove all observers
     */
    void clearObservers() {
        observers_.clear();
    }

    /**
     * Detect changes between old and new wishlist item state
     * Returns detected changes and notifies all observers
     */
    [[nodiscard]] std::vector<ChangeEvent> detectChanges(
        const WishlistItem& old_item,
        const WishlistItem& new_item
    ) {
        std::vector<ChangeEvent> changes;
        const auto now = std::chrono::system_clock::now();

        // Check if price dropped below threshold
        if (new_item.notify_on_price_drop &&
            new_item.in_stock &&
            new_item.current_price <= new_item.desired_max_price &&
            old_item.current_price > new_item.desired_max_price) {

            ChangeEvent event{
                .type = ChangeType::PriceDroppedBelowThreshold,
                .item = new_item,
                .old_price = old_item.current_price,
                .new_price = new_item.current_price,
                .detected_at = now
            };
            changes.push_back(event);
            notifyObservers(event);
        }
        // Check if back in stock
        else if (new_item.notify_on_stock &&
                 !old_item.in_stock &&
                 new_item.in_stock) {

            ChangeEvent event{
                .type = ChangeType::BackInStock,
                .item = new_item,
                .old_stock_status = old_item.in_stock,
                .new_stock_status = new_item.in_stock,
                .detected_at = now
            };
            changes.push_back(event);
            notifyObservers(event);
        }
        // Check if price changed (informational, always track)
        else if (std::abs(old_item.current_price - new_item.current_price) > 0.01) {
            ChangeEvent event{
                .type = ChangeType::PriceChanged,
                .item = new_item,
                .old_price = old_item.current_price,
                .new_price = new_item.current_price,
                .detected_at = now
            };
            changes.push_back(event);
            // Note: We don't notify observers for minor price changes
        }
        // Check if out of stock
        else if (old_item.in_stock && !new_item.in_stock) {
            ChangeEvent event{
                .type = ChangeType::OutOfStock,
                .item = new_item,
                .old_stock_status = old_item.in_stock,
                .new_stock_status = new_item.in_stock,
                .detected_at = now
            };
            changes.push_back(event);
            // Note: We don't notify for out of stock events
        }

        return changes;
    }

private:
    void notifyObservers(const ChangeEvent& event) {
        for (auto& observer : observers_) {
            if (observer) {
                observer->onChangeDetected(event);
            }
        }
    }

    std::vector<std::shared_ptr<IChangeObserver>> observers_;
};

} // namespace bluray::domain
