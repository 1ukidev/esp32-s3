#include "web_server.hpp"

#include <array>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <string>
#include <string_view>

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
    config.max_uri_handlers = 8;
    config.stack_size = 6144;
    config.lru_purge_enable = true;

    port_ = port;
    max_streams_ = max_streams;
    html_name_ = escape_html(name);
    streaming_ = true;
    if (const esp_err_t err = httpd_start(&handle_, &config); err != ESP_OK) {
        handle_ = nullptr;
        streaming_ = false;
        return err;
    }

    const std::array routes{
        httpd_uri_t{.uri = "/", .method = HTTP_GET, .handler = &handle_index, .user_ctx = this},
        httpd_uri_t{.uri = "/stream", .method = HTTP_GET, .handler = &handle_stream, .user_ctx = this},
        httpd_uri_t{.uri = "/capture", .method = HTTP_GET, .handler = &handle_capture, .user_ctx = this},
        httpd_uri_t{.uri = "/api/settings", .method = HTTP_GET, .handler = &handle_settings, .user_ctx = this},
        httpd_uri_t{.uri = "/api/control", .method = HTTP_POST, .handler = &handle_control, .user_ctx = this},
        httpd_uri_t{.uri = "/api/system", .method = HTTP_GET, .handler = &handle_system, .user_ctx = this},
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

    streaming_ = false;
    for (auto waited = std::chrono::milliseconds{0}; streams_ > 0 && waited < stop_timeout; waited += stop_poll) {
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

esp_err_t WebServer::handle_stream(httpd_req_t* request)
{
    auto& self = server_of(request);
    if (self.streams_.fetch_add(1) >= self.max_streams_ || !self.streaming_) {
        --self.streams_;
        httpd_resp_set_status(request, "503 Service Unavailable");
        return httpd_resp_sendstr(request, "Concurrent stream limit reached");
    }

    // The stream runs on its own task so the server keeps answering other requests.
    httpd_req_t* async_request = nullptr;
    if (const esp_err_t err = httpd_req_async_handler_begin(request, &async_request); err != ESP_OK) {
        --self.streams_;
        return err;
    }
    if (xTaskCreate(&stream_task, "cam_stream", 4096, async_request, 5, nullptr) != pdPASS) {
        --self.streams_;
        httpd_req_async_handler_complete(async_request);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void WebServer::stream_task(void* argument)
{
    auto* request = static_cast<httpd_req_t*>(argument);
    auto& self = server_of(request);
    self.stream(request);
    httpd_req_async_handler_complete(request);
    --self.streams_;
    vTaskDelete(nullptr);
}

void WebServer::stream(httpd_req_t* request)
{
    httpd_resp_set_type(request, stream_content_type);
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");

    const int no_delay = 1;
    setsockopt(httpd_req_to_sockfd(request), IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));

    std::array<char, 96> header{};
    while (streaming_) {
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

}  // namespace camera_server
