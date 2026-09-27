#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

#include "esp_err.h"
#include "esp_http_server.h"

#include "camera.hpp"

namespace camera_server {

class FrameRate {
public:
    void tick();
    [[nodiscard]] std::uint32_t value() const;

private:
    mutable std::mutex mutex_;
    std::int64_t second_ = 0;
    std::uint32_t current_ = 0;
    std::uint32_t previous_ = 0;
};

class WebServer {
public:
    struct Stats {
        std::uint16_t port = 0;
        std::uint32_t uptime_s = 0;
        std::size_t streams = 0;
        std::size_t max_streams = 0;
        std::uint32_t fps = 0;
        std::uint64_t frames = 0;
        std::uint64_t bytes = 0;
    };

    explicit WebServer(Camera& camera) : camera_(camera) {}
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;
    ~WebServer() { stop(); }

    esp_err_t start(std::uint16_t port, std::size_t max_streams, std::string_view name);
    void stop();

    [[nodiscard]] bool running() const { return handle_ != nullptr; }
    [[nodiscard]] Stats stats() const;

private:
    static esp_err_t handle_index(httpd_req_t* request);
    static esp_err_t handle_capture(httpd_req_t* request);
    static esp_err_t handle_stream(httpd_req_t* request);
    static esp_err_t handle_settings(httpd_req_t* request);
    static esp_err_t handle_control(httpd_req_t* request);
    static esp_err_t handle_system(httpd_req_t* request);
    static void stream_task(void* request);

    void stream(httpd_req_t* request);

    Camera& camera_;
    httpd_handle_t handle_ = nullptr;
    std::uint16_t port_ = 0;
    std::size_t max_streams_ = 0;
    std::string html_name_;
    std::int64_t started_at_ = 0;
    std::atomic<bool> streaming_{false};
    std::atomic<std::size_t> streams_{0};
    std::atomic<std::uint64_t> frames_{0};
    std::atomic<std::uint64_t> bytes_{0};
    FrameRate frame_rate_;
};

}  // namespace camera_server
