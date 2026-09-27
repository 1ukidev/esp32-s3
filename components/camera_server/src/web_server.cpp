#include "web_server.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#include "json.hpp"
#include "network.hpp"
#include "web_ui.hpp"

namespace camera_server {
namespace {

constexpr const char* TAG = "web";

#define PART_BOUNDARY "camera-server-frame"
constexpr const char* stream_content_type = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
constexpr const char* stream_part = "\r\n--" PART_BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";
#undef PART_BOUNDARY

constexpr std::size_t max_detections = 2;
constexpr std::size_t max_upload_size = 4 * 1024 * 1024;
constexpr std::size_t max_detector_url_body = 512;

constexpr auto stop_timeout = std::chrono::seconds{8};
constexpr auto stop_poll = std::chrono::milliseconds{50};

WebServer& server_of(httpd_req_t* request)
{
    return *static_cast<WebServer*>(request->user_ctx);
}

esp_err_t send_json(httpd_req_t* request, std::string_view json)
{
    httpd_resp_set_type(request, "application/json");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, json.data(), static_cast<ssize_t>(json.size()));
}

esp_err_t send_error(httpd_req_t* request, httpd_err_code_t code, const char* message)
{
    httpd_resp_send_err(request, code, message);
    return ESP_OK;
}

esp_err_t send_status(httpd_req_t* request, const char* status, const char* message)
{
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "text/plain");
    return httpd_resp_sendstr(request, message);
}

bool receive_body(httpd_req_t* request, std::span<char> body)
{
    while (!body.empty()) {
        const int received = httpd_req_recv(request, body.data(), body.size());
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (received <= 0) {
            return false;
        }
        body = body.subspan(static_cast<std::size_t>(received));
    }
    return true;
}

std::string_view trim(std::string_view text)
{
    constexpr std::string_view blanks = " \t\r\n";
    const auto first = text.find_first_not_of(blanks);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(blanks) - first + 1);
}

std::string escape_html(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        case '\'': escaped += "&#39;"; break;
        default: escaped += c;
        }
    }
    return escaped;
}

esp_err_t send_chunk(httpd_req_t* request, std::string_view chunk)
{
    return httpd_resp_send_chunk(request, chunk.data(), static_cast<ssize_t>(chunk.size()));
}

template <std::size_t N>
bool query_value(std::string_view query, const char* key, std::array<char, N>& value)
{
    return httpd_query_key_value(query.data(), key, value.data(), value.size()) == ESP_OK;
}

}  // namespace

void FrameRate::tick()
{
    const std::int64_t second = esp_timer_get_time() / 1'000'000;
    std::scoped_lock lock(mutex_);
    if (second != second_) {
        previous_ = second == second_ + 1 ? current_ : 0;
        current_ = 0;
        second_ = second;
    }
    ++current_;
}

std::uint32_t FrameRate::value() const
{
    const std::int64_t second = esp_timer_get_time() / 1'000'000;
    std::scoped_lock lock(mutex_);
    if (second == second_) {
        return previous_;
    }
    return second == second_ + 1 ? current_ : 0;
}

esp_err_t WebServer::start(std::uint16_t port, std::size_t max_streams, std::string_view name)
{
    if (running()) {
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_uri_handlers = 12;
    config.stack_size = 6144;
    config.lru_purge_enable = true;

    port_ = port;
    max_streams_ = max_streams;
    html_name_ = escape_html(name);
    accepting_ = true;
    if (const esp_err_t err = httpd_start(&handle_, &config); err != ESP_OK) {
        handle_ = nullptr;
        accepting_ = false;
        return err;
    }

    const std::array routes{
        httpd_uri_t{.uri = "/", .method = HTTP_GET, .handler = &handle_index, .user_ctx = this},
        httpd_uri_t{.uri = "/stream", .method = HTTP_GET, .handler = &handle_stream, .user_ctx = this},
        httpd_uri_t{.uri = "/capture", .method = HTTP_GET, .handler = &handle_capture, .user_ctx = this},
        httpd_uri_t{.uri = "/api/settings", .method = HTTP_GET, .handler = &handle_settings, .user_ctx = this},
        httpd_uri_t{.uri = "/api/control", .method = HTTP_POST, .handler = &handle_control, .user_ctx = this},
        httpd_uri_t{.uri = "/api/reset", .method = HTTP_POST, .handler = &handle_reset, .user_ctx = this},
        httpd_uri_t{.uri = "/api/system", .method = HTTP_GET, .handler = &handle_system, .user_ctx = this},
        httpd_uri_t{.uri = "/api/detect", .method = HTTP_GET, .handler = &handle_detect, .user_ctx = this},
        httpd_uri_t{.uri = "/api/detect", .method = HTTP_POST, .handler = &handle_detect, .user_ctx = this},
        httpd_uri_t{.uri = "/api/detector", .method = HTTP_GET, .handler = &handle_detector_get, .user_ctx = this},
        httpd_uri_t{.uri = "/api/detector", .method = HTTP_POST, .handler = &handle_detector_set, .user_ctx = this},
    };
    for (const auto& route : routes) {
        httpd_register_uri_handler(handle_, &route);
    }

    frames_ = 0;
    bytes_ = 0;
    started_at_ = esp_timer_get_time();
    ESP_LOGI(TAG, "Web server listening on port %u", port);
    return ESP_OK;
}

void WebServer::stop()
{
    if (!running()) {
        return;
    }

    accepting_ = false;
    for (auto waited = std::chrono::milliseconds{0}; (streams_ > 0 || detections_ > 0) && waited < stop_timeout;
         waited += stop_poll) {
        vTaskDelay(pdMS_TO_TICKS(stop_poll.count()));
    }

    httpd_stop(handle_);
    handle_ = nullptr;
    ESP_LOGI(TAG, "Web server stopped");
}

WebServer::Stats WebServer::stats() const
{
    if (!running()) {
        return {};
    }
    return {
        .port = port_,
        .uptime_s = static_cast<std::uint32_t>((esp_timer_get_time() - started_at_) / 1'000'000),
        .streams = streams_,
        .max_streams = max_streams_,
        .fps = frame_rate_.value(),
        .frames = frames_,
        .bytes = bytes_,
    };
}

esp_err_t WebServer::handle_index(httpd_req_t* request)
{
    constexpr std::string_view placeholder = "{{name}}";
    const auto& name = server_of(request).html_name_;

    httpd_resp_set_type(request, "text/html; charset=utf-8");
    std::string_view rest = index_html;
    for (auto at = rest.find(placeholder); at != std::string_view::npos; at = rest.find(placeholder)) {
        if (send_chunk(request, rest.substr(0, at)) != ESP_OK || send_chunk(request, name) != ESP_OK) {
            return ESP_FAIL;
        }
        rest.remove_prefix(at + placeholder.size());
    }
    if (send_chunk(request, rest) != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(request, nullptr, 0);
}

esp_err_t WebServer::handle_capture(httpd_req_t* request)
{
    const auto frame = server_of(request).camera_.capture();
    if (!frame) {
        return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to capture image");
    }

    httpd_resp_set_type(request, "image/jpeg");
    httpd_resp_set_hdr(request, "Content-Disposition", "inline; filename=capture.jpg");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    return httpd_resp_send(request, reinterpret_cast<const char*>(frame->buf), static_cast<ssize_t>(frame->len));
}

struct WebServer::AsyncContext {
    httpd_req_t* request;
    Job job;
    std::atomic<std::size_t>* active;
};

esp_err_t WebServer::run_async(httpd_req_t* request, Job job, std::atomic<std::size_t>& active, std::size_t limit,
                               const char* task_name, std::uint32_t stack_size)
{
    if (active.fetch_add(1) >= limit || !accepting_) {
        --active;
        httpd_resp_set_hdr(request, "Retry-After", "1");
        return send_status(request, "503 Service Unavailable", "Too many concurrent requests");
    }

    httpd_req_t* async_request = nullptr;
    if (const esp_err_t err = httpd_req_async_handler_begin(request, &async_request); err != ESP_OK) {
        --active;
        return err;
    }
    auto* context = new (std::nothrow) AsyncContext{async_request, job, &active};
    if (context == nullptr || xTaskCreate(&async_task, task_name, stack_size, context, 5, nullptr) != pdPASS) {
        delete context;
        --active;
        httpd_req_async_handler_complete(async_request);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void WebServer::async_task(void* argument)
{
    const AsyncContext context = *static_cast<AsyncContext*>(argument);
    delete static_cast<AsyncContext*>(argument);

    auto& self = server_of(context.request);
    (self.*context.job)(context.request);
    httpd_req_async_handler_complete(context.request);
    --*context.active;
    vTaskDelete(nullptr);
}

esp_err_t WebServer::handle_stream(httpd_req_t* request)
{
    auto& self = server_of(request);
    return self.run_async(request, &WebServer::stream, self.streams_, self.max_streams_, "cam_stream", 4096);
}

void WebServer::stream(httpd_req_t* request)
{
    httpd_resp_set_type(request, stream_content_type);
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");

    const int no_delay = 1;
    setsockopt(httpd_req_to_sockfd(request), IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));

    std::array<char, 96> header{};
    while (accepting_) {
        const auto frame = camera_.capture();
        if (!frame) {
            ESP_LOGW(TAG, "Frame dropped");
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        const int header_length = std::snprintf(header.data(), header.size(), stream_part,
                                                static_cast<unsigned>(frame->len));
        if (httpd_resp_send_chunk(request, header.data(), header_length) != ESP_OK ||
            httpd_resp_send_chunk(request, reinterpret_cast<const char*>(frame->buf),
                                  static_cast<ssize_t>(frame->len)) != ESP_OK) {
            return;
        }

        ++frames_;
        bytes_ += frame->len;
        frame_rate_.tick();
    }
    httpd_resp_send_chunk(request, nullptr, 0);
}

esp_err_t WebServer::handle_settings(httpd_req_t* request)
{
    return send_json(request, server_of(request).camera_.settings_json());
}

esp_err_t WebServer::handle_control(httpd_req_t* request)
{
    std::array<char, 128> query{};
    std::array<char, 32> name{};
    std::array<char, 16> text{};
    if (httpd_req_get_url_query_str(request, query.data(), query.size()) != ESP_OK ||
        !query_value(query.data(), "var", name) || !query_value(query.data(), "val", text)) {
        return send_error(request, HTTPD_400_BAD_REQUEST, "Parameters 'var' and 'val' are required");
    }

    const std::string_view value_text{text.data()};
    int value = 0;
    if (const auto [end, ec] = std::from_chars(value_text.begin(), value_text.end(), value);
        ec != std::errc{} || end != value_text.end()) {
        return send_error(request, HTTPD_400_BAD_REQUEST, "Invalid value");
    }

    auto& camera = server_of(request).camera_;
    switch (camera.set(name.data(), value)) {
    case ESP_OK:
        return send_json(request, camera.settings_json());
    case ESP_ERR_NOT_FOUND:
        return send_error(request, HTTPD_404_NOT_FOUND, "Unknown control");
    case ESP_ERR_INVALID_ARG:
        return send_error(request, HTTPD_400_BAD_REQUEST, "Value out of range");
    default:
        return send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "The sensor rejected the setting");
    }
}

esp_err_t WebServer::handle_reset(httpd_req_t* request)
{
    auto& camera = server_of(request).camera_;
    camera.reset();
    return send_json(request, camera.settings_json());
}

esp_err_t WebServer::handle_system(httpd_req_t* request)
{
    const auto stats = server_of(request).stats();
    const auto network = network_info();

    return send_json(request, JsonObject{}
                                  .add("uptime", stats.uptime_s)
                                  .add("streams", stats.streams)
                                  .add("max_streams", stats.max_streams)
                                  .add("fps", stats.fps)
                                  .add("frames", stats.frames)
                                  .add("bytes", stats.bytes)
                                  .add("heap_free", heap_caps_get_free_size(MALLOC_CAP_INTERNAL))
                                  .add("heap_min", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL))
                                  .add("psram_free", heap_caps_get_free_size(MALLOC_CAP_SPIRAM))
                                  .add("psram_total", heap_caps_get_total_size(MALLOC_CAP_SPIRAM))
                                  .add("ssid", network ? network->ssid : std::string{})
                                  .add("ip", network ? network->ip : std::string{})
                                  .add("rssi", network ? network->rssi : 0)
                                  .str());
}

esp_err_t WebServer::handle_detect(httpd_req_t* request)
{
    auto& self = server_of(request);
    // TLS handshakes with an https:// API need a larger stack.
    return self.run_async(request, &WebServer::detect, self.detections_, max_detections, "cam_detect", 10240);
}

void WebServer::detect(httpd_req_t* request)
{
    std::vector<std::uint8_t> image;
    std::size_t width = 0;
    std::size_t height = 0;

    if (request->method == HTTP_POST) {
        if (request->content_len == 0 || request->content_len > max_upload_size) {
            send_status(request, "413 Content Too Large", "Send a JPEG image of up to 4 MB");
            return;
        }
        image.resize(request->content_len);
        if (!receive_body(request, {reinterpret_cast<char*>(image.data()), image.size()})) {
            return;
        }
    } else {
        const auto frame = camera_.capture();
        if (!frame) {
            send_error(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to capture image");
            return;
        }
        image.assign(frame->buf, frame->buf + frame->len);
        width = frame->width;
        height = frame->height;
    }

    const std::int64_t started = esp_timer_get_time();
    const auto result = detector_.detect(image);
    if (!result) {
        switch (result.error()) {
        case ESP_ERR_INVALID_STATE:
            send_status(request, "409 Conflict", "Detector API not configured");
            break;
        case ESP_ERR_INVALID_RESPONSE:
        case ESP_ERR_INVALID_SIZE:
            send_status(request, "502 Bad Gateway", "Invalid response from the detector API");
            break;
        default:
            send_status(request, "504 Gateway Timeout", "Detector API unreachable");
        }
        return;
    }

    JsonObject json;
    json.add("latency_ms", static_cast<std::uint32_t>((esp_timer_get_time() - started) / 1000));
    if (width > 0) {
        json.add("width", width).add("height", height);
    }
    send_json(request, json.add_raw("result", *result).str());
}

esp_err_t WebServer::handle_detector_get(httpd_req_t* request)
{
    return send_json(request, JsonObject{}.add("url", server_of(request).detector_.url()).str());
}

esp_err_t WebServer::handle_detector_set(httpd_req_t* request)
{
    if (request->content_len > max_detector_url_body) {
        return send_error(request, HTTPD_400_BAD_REQUEST, "URL too long");
    }
    std::string body(request->content_len, '\0');
    if (!receive_body(request, body)) {
        return ESP_FAIL;
    }

    auto& detector = server_of(request).detector_;
    if (detector.set_url(trim(body)) != ESP_OK) {
        return send_error(request, HTTPD_400_BAD_REQUEST, "Expected an http:// or https:// URL");
    }
    return handle_detector_get(request);
}

}  // namespace camera_server
