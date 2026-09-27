idf_component_register(
    SRCS
        "src/camera.cpp"
        "src/camera_server.cpp"
        "src/console.cpp"
        "src/network.cpp"
        "src/web_server.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    PRIV_REQUIRES
        console
        esp_event
        esp_http_server
        esp_netif
        esp_timer
        lwip
        esp_wifi
)
