idf_component_register(
    SRCS "microphone.cpp" "main.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver freertos esp_system esp_adc 
)
