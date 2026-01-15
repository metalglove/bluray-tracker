#pragma once

#include "../../domain/models.hpp"
#include "../../domain/change_detector.hpp"
#include <string>
#include <memory>

namespace bluray::application::notifier {

/**
 * Abstract notifier interface (Strategy pattern)
 */
class INotifier : public domain::IChangeObserver {
public:
    virtual ~INotifier() = default;

    /**
     * Send notification about change event
     */
    virtual void notify(const domain::ChangeEvent& event) = 0;

    /**
     * IChangeObserver implementation
     */
    void onChangeDetected(const domain::ChangeEvent& event) override {
        notify(event);
    }

    /**
     * Check if notifier is configured and ready
     */
    virtual bool isConfigured() const = 0;
};

} // namespace bluray::application::notifier
