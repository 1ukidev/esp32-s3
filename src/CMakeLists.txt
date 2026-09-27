idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES camera_server chip_temperature console wifi_manager
)
