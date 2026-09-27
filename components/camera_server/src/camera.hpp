#pragma once

#include <memory>
#include <string>
#include <string_view>

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

    esp_err_t set(std::string_view control, int value);
    [[nodiscard]] std::string settings_json() const;

private:
    sensor_t* sensor_ = nullptr;
    framesize_t max_frame_size_ = FRAMESIZE_INVALID;
    std::string_view model_{};
};

}  // namespace camera_server
