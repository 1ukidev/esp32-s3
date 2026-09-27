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
#include "detector.hpp"

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

    WebServer(Camera& camera, Detector& detector) : camera_(camera), detector_(detector) {}
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
    static esp_err_t handle_reset(httpd_req_t* request);
    static esp_err_t handle_system(httpd_req_t* request);
    static esp_err_t handle_detect(httpd_req_t* request);
    static esp_err_t handle_detector_get(httpd_req_t* request);
    static esp_err_t handle_detector_set(httpd_req_t* request);

    using Job = void (WebServer::*)(httpd_req_t*);
    // Moves a long-running request to its own task so the server keeps answering others.
    esp_err_t run_async(httpd_req_t* request, Job job, std::atomic<std::size_t>& active, std::size_t limit,
                        const char* task_name, std::uint32_t stack_size);
    static void async_task(void* context);
    struct AsyncContext;

    void stream(httpd_req_t* request);
    void detect(httpd_req_t* request);

    Camera& camera_;
    Detector& detector_;
    httpd_handle_t handle_ = nullptr;
    std::uint16_t port_ = 0;
    std::size_t max_streams_ = 0;
    std::string html_name_;
    std::int64_t started_at_ = 0;
    std::atomic<bool> accepting_{false};
    std::atomic<std::size_t> streams_{0};
    std::atomic<std::size_t> detections_{0};
    std::atomic<std::uint64_t> frames_{0};
    std::atomic<std::uint64_t> bytes_{0};
    FrameRate frame_rate_;
};

}  // namespace camera_server
