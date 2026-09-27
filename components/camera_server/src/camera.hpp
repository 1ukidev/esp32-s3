#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "esp_camera.h"
#include "esp_err.h"

#include "camera_server/camera_server.hpp"

namespace camera_server {

struct FrameReturner {
    void operator()(camera_fb_t* frame) const noexcept { esp_camera_fb_return(frame); }
};

using Frame = std::unique_ptr<camera_fb_t, FrameReturner>;

class Camera {
public:
    esp_err_t init(const Config& config);
    void deinit();

    [[nodiscard]] bool ready() const { return sensor_ != nullptr; }
    [[nodiscard]] Frame capture() const { return Frame{esp_camera_fb_get()}; }
    [[nodiscard]] std::string_view model() const { return model_; }
    [[nodiscard]] framesize_t frame_size() const;

    // Applies and persists a sensor setting; saved settings are restored on init().
    esp_err_t set(std::string_view control, int value);
    // Restores the values the sensor had right after init() and forgets the saved ones.
    esp_err_t reset();
    [[nodiscard]] std::string settings_json() const;

private:
    [[nodiscard]] bool in_range(std::string_view control, int value) const;
    void restore_saved();

    sensor_t* sensor_ = nullptr;
    std::vector<int> defaults_;
    framesize_t max_frame_size_ = FRAMESIZE_INVALID;
    std::string_view model_{};
};

}  // namespace camera_server
