#pragma once

#include <cstdint>
#include <expected>
#include <mutex>
#include <span>
#include <string>
#include <string_view>

#include "esp_err.h"

namespace camera_server {

// Client for an external YOLO HTTP API that accepts a multipart "file" upload
// and answers with {"detections": [{"class", "confidence", "bbox": [x1, y1, x2, y2]}]}.
class Detector {
public:
    void set_default_url(std::string_view url);

    [[nodiscard]] std::string url();
    esp_err_t set_url(std::string_view url);

    // Returns the API's JSON response as-is.
    [[nodiscard]] std::expected<std::string, esp_err_t> detect(std::span<const std::uint8_t> jpeg);

private:
    void load();

    std::mutex mutex_;
    std::string url_;
    std::string default_url_;
    bool loaded_ = false;
};

[[nodiscard]] bool is_valid_detector_url(std::string_view url);

}  // namespace camera_server
