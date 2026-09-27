#include "detector.hpp"

#include <cctype>
#include <memory>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_log.h"

#include "storage.hpp"

namespace camera_server {
namespace {

constexpr const char* TAG = "detector";
constexpr const char* nvs_namespace = "camera_server";
constexpr const char* nvs_key = "detector_url";
constexpr int timeout_ms = 5000;
constexpr std::size_t max_url_length = 256;
constexpr std::size_t max_response_size = 64 * 1024;
constexpr std::string_view boundary = "camera-server-upload";

struct ClientDeleter {
    void operator()(esp_http_client* client) const noexcept { esp_http_client_cleanup(client); }
};
using Client = std::unique_ptr<esp_http_client, ClientDeleter>;

bool write_all(esp_http_client_handle_t client, std::span<const char> data)
{
    while (!data.empty()) {
        const int written = esp_http_client_write(client, data.data(), static_cast<int>(data.size()));
        if (written <= 0) {
            return false;
        }
        data = data.subspan(static_cast<std::size_t>(written));
    }
    return true;
}

std::span<const char> as_chars(std::string_view text)
{
    return {text.data(), text.size()};
}

}  // namespace

bool is_valid_detector_url(std::string_view url)
{
    return url.size() <= max_url_length && (url.starts_with("http://") || url.starts_with("https://"));
}

void Detector::set_default_url(std::string_view url)
{
    std::scoped_lock lock(mutex_);
    default_url_ = url;
}

void Detector::load()
{
    if (loaded_) {
        return;
    }
    loaded_ = true;
    url_ = default_url_;

    const auto store = NvsNamespace::open(nvs_namespace, NVS_READONLY);
    std::size_t length = 0;
    if (!store || nvs_get_str(store->get(), nvs_key, nullptr, &length) != ESP_OK || length == 0) {
        return;
    }
    std::string stored(length, '\0');
    if (nvs_get_str(store->get(), nvs_key, stored.data(), &length) == ESP_OK) {
        stored.resize(length - 1);
        url_ = std::move(stored);
    }
}

std::string Detector::url()
{
    std::scoped_lock lock(mutex_);
    load();
    return url_;
}

esp_err_t Detector::set_url(std::string_view url)
{
    if (!url.empty() && !is_valid_detector_url(url)) {
        return ESP_ERR_INVALID_ARG;
    }

    std::scoped_lock lock(mutex_);
    load();

    const auto store = NvsNamespace::open(nvs_namespace, NVS_READWRITE);
    if (!store) {
        return ESP_ERR_NVS_NOT_INITIALIZED;
    }

    url_ = url;
    if (const esp_err_t err = nvs_set_str(store->get(), nvs_key, url_.c_str()); err != ESP_OK) {
        return err;
    }
    return nvs_commit(store->get());
}

std::expected<std::string, esp_err_t> Detector::detect(std::span<const std::uint8_t> jpeg)
{
    const std::string endpoint = url();
    if (endpoint.empty()) {
        return std::unexpected(ESP_ERR_INVALID_STATE);
    }

    esp_http_client_config_t config{};
    config.url = endpoint.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = timeout_ms;
    config.crt_bundle_attach = esp_crt_bundle_attach;

    const Client client{esp_http_client_init(&config)};
    if (!client) {
        return std::unexpected(ESP_ERR_NO_MEM);
    }

    const std::string head = std::string{"--"}.append(boundary).append(
        "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"frame.jpg\"\r\n"
        "Content-Type: image/jpeg\r\n\r\n");
    const std::string tail = std::string{"\r\n--"}.append(boundary).append("--\r\n");
    const std::string content_type = std::string{"multipart/form-data; boundary="}.append(boundary);
    esp_http_client_set_header(client.get(), "Content-Type", content_type.c_str());

    const auto body_size = head.size() + jpeg.size() + tail.size();
    if (const esp_err_t err = esp_http_client_open(client.get(), static_cast<int>(body_size)); err != ESP_OK) {
        ESP_LOGW(TAG, "Could not reach %s: %s", endpoint.c_str(), esp_err_to_name(err));
        return std::unexpected(err);
    }

    const std::span<const char> image{reinterpret_cast<const char*>(jpeg.data()), jpeg.size()};
    if (!write_all(client.get(), as_chars(head)) || !write_all(client.get(), image) ||
        !write_all(client.get(), as_chars(tail)) || esp_http_client_fetch_headers(client.get()) < 0) {
        ESP_LOGW(TAG, "Upload to %s failed", endpoint.c_str());
        return std::unexpected(ESP_ERR_TIMEOUT);
    }

    std::string response;
    char buffer[1024];
    for (int read = 0; (read = esp_http_client_read(client.get(), buffer, sizeof(buffer))) > 0;) {
        if (response.size() + static_cast<std::size_t>(read) > max_response_size) {
            return std::unexpected(ESP_ERR_INVALID_SIZE);
        }
        response.append(buffer, static_cast<std::size_t>(read));
    }

    if (const int status = esp_http_client_get_status_code(client.get()); status != 200) {
        ESP_LOGW(TAG, "API answered with HTTP %d", status);
        return std::unexpected(ESP_ERR_INVALID_RESPONSE);
    }

    while (!response.empty() && std::isspace(static_cast<unsigned char>(response.back()))) {
        response.pop_back();
    }
    if (!response.starts_with('{') || !response.ends_with('}')) {
        return std::unexpected(ESP_ERR_INVALID_RESPONSE);
    }
    return response;
}

}  // namespace camera_server
