#include "image_cache.hpp"
#include "logger.hpp"
#include <fmt/format.h>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <sstream>

namespace bluray::infrastructure {

ImageCache::ImageCache(std::string_view cache_directory)
    : cache_dir_(cache_directory) {

  // Create cache directory if it doesn't exist
  if (!std::filesystem::exists(cache_dir_)) {
    std::filesystem::create_directories(cache_dir_);
    Logger::instance().info(
        fmt::format("Created cache directory: {}", cache_dir_.string()));
  }
}

std::optional<std::string> ImageCache::cacheImage(std::string_view image_url) {
  if (image_url.empty()) {
    return std::nullopt;
  }

  std::lock_guard<std::mutex> lock(mutex_);

  // Check if already cached
  auto cached_path = getCachedPath(image_url);
  if (cached_path) {
    return cached_path;
  }

  // Download image
  NetworkClient client;
  auto image_data = client.downloadFile(image_url);

  if (!image_data || image_data->empty()) {
    Logger::instance().warning(
        fmt::format("Failed to download image: {}", image_url));
    return std::nullopt;
  }

  // Generate filename
  const std::string filename = generateFilename(image_url);
  const auto file_path = cache_dir_ / filename;

  // Save to disk
  std::ofstream file(file_path, std::ios::binary);
  if (!file.is_open()) {
    Logger::instance().error(
        fmt::format("Failed to open file for writing: {}", file_path.string()));
    return std::nullopt;
  }

  file.write(reinterpret_cast<const char *>(image_data->data()),
             static_cast<std::streamsize>(image_data->size()));
  file.close();

  Logger::instance().debug(
      fmt::format("Cached image: {} -> {}", image_url, file_path.string()));

  return file_path.string();
}

std::optional<std::string>
ImageCache::getCachedPath(std::string_view image_url) const {
  if (image_url.empty()) {
    return std::nullopt;
  }

  const std::string filename = generateFilename(image_url);
  const auto file_path = cache_dir_ / filename;

  if (std::filesystem::exists(file_path)) {
    return file_path.string();
  }

  return std::nullopt;
}

bool ImageCache::isCached(std::string_view image_url) const {
  return getCachedPath(image_url).has_value();
}

void ImageCache::clear() {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!std::filesystem::exists(cache_dir_)) {
    return;
  }

  size_t count = 0;
  for (const auto &entry : std::filesystem::directory_iterator(cache_dir_)) {
    if (entry.is_regular_file()) {
      std::filesystem::remove(entry.path());
      ++count;
    }
  }

  Logger::instance().info(fmt::format("Cleared {} cached images", count));
}

std::string ImageCache::generateFilename(std::string_view url) const {
  // Generate SHA256 hash of URL
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(reinterpret_cast<const unsigned char *>(url.data()), url.size(), hash);

  // Convert to hex string
  std::ostringstream oss;
  for (unsigned char byte : hash) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(byte);
  }

  // Detect file extension
  const std::string extension = detectExtension(url);

  return oss.str() + extension;
}

std::string ImageCache::detectExtension(std::string_view url) const {
  // Try to detect extension from URL
  const size_t dot_pos = url.find_last_of('.');
  const size_t query_pos = url.find('?', dot_pos);

  if (dot_pos != std::string_view::npos) {
    size_t end_pos =
        query_pos != std::string_view::npos ? query_pos : url.size();
    std::string ext(url.substr(dot_pos, end_pos - dot_pos));

    // Validate extension (common image formats)
    if (ext == ".jpg" || ext == ".jpeg" || ext == ".png" || ext == ".gif" ||
        ext == ".webp" || ext == ".bmp") {
      return ext;
    }
  }

  // Default to .jpg
  return ".jpg";
}

} // namespace bluray::infrastructure
