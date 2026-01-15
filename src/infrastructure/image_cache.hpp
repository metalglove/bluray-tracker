#pragma once

#include "network_client.hpp"
#include <string>
#include <string_view>
#include <filesystem>
#include <optional>
#include <mutex>

namespace bluray::infrastructure {

/**
 * Image cache manager that downloads and stores product images locally
 */
class ImageCache {
public:
    explicit ImageCache(std::string_view cache_directory);

    /**
     * Download and cache an image from URL
     * Returns the local file path on success
     */
    [[nodiscard]] std::optional<std::string> cacheImage(std::string_view image_url);

    /**
     * Get local path for cached image
     * Returns nullopt if image is not cached
     */
    [[nodiscard]] std::optional<std::string> getCachedPath(std::string_view image_url) const;

    /**
     * Check if image is already cached
     */
    [[nodiscard]] bool isCached(std::string_view image_url) const;

    /**
     * Clear all cached images
     */
    void clear();

    /**
     * Get cache directory path
     */
    [[nodiscard]] const std::filesystem::path& getCacheDirectory() const {
        return cache_dir_;
    }

private:
    [[nodiscard]] std::string generateFilename(std::string_view url) const;
    [[nodiscard]] std::string detectExtension(std::string_view url) const;

    std::filesystem::path cache_dir_;
    mutable std::mutex mutex_;
};

} // namespace bluray::infrastructure
