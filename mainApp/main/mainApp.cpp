#include "mainApp.h"
#include <iostream>
#include <stdexcept>

MainApp::MainApp()
    : currentState(State::OFF),
      currentLanguage("English"),
      currentFontSize("Medium")
{
    availableLanguages = {"English", "Spanish", "Hindi", "Mandarin", "Arabic", "French", "Portugues"};
    availableFontSizes = {"8", "12", "16", "20", "24"};
    initLeds();
}

void MainApp::initLeds() {
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED1_PIN) | (1ULL << LED2_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    gpio_set_level(LED1_PIN, 0);
    gpio_set_level(LED2_PIN, 0);
}

MainApp::State MainApp::getState() const {
    return currentState;
}

std::string MainApp::getLanguage() const {
    return currentLanguage;
}

std::string MainApp::getFontSize() const {
    return currentFontSize;
}

std::vector<std::string> MainApp::getAvailableLanguages() const {
    return availableLanguages;
}

std::vector<std::string> MainApp::getAvailableFontSizes() const {
    return availableFontSizes;
}

void MainApp::setState(State newState) {
    currentState = newState;
}

void MainApp::setLanguage(const std::string& lang) {
    if (!isValidOption(lang, availableLanguages)) {
        ESP_LOGE(TAG, "Unsupported language: %s", lang.c_str());
        return;
    }
    currentLanguage = lang;
    ESP_LOGI(TAG, "Language set to: %s", lang.c_str());
}

void MainApp::setFontSize(const std::string& size) {
    if (!isValidOption(size, availableFontSizes)) {
        ESP_LOGE(TAG, "Unsupported font size: %s", size.c_str());
        return;
    }
    currentFontSize = size;
    ESP_LOGI(TAG, "Font size set to: %s", size.c_str());
}

void MainApp::setAvailableLanguages(const std::vector<std::string>& langs) {
    availableLanguages = langs;
    ESP_LOGI(TAG, "Available languages updated (count: %d)", static_cast<int>(langs.size()));
    if (!isValidOption(currentLanguage, availableLanguages) && !langs.empty()) {
        currentLanguage = langs[0];
        ESP_LOGW(TAG, "Current language not in list, defaulting to: %s", currentLanguage.c_str());
    }
}

void MainApp::setAvailableFontSizes(const std::vector<std::string>& sizes) {
    availableFontSizes = sizes;
    ESP_LOGI(TAG, "Available font sizes updated (count: %d)", static_cast<int>(sizes.size()));
    if (!isValidOption(currentFontSize, availableFontSizes) && !sizes.empty()) {
        currentFontSize = sizes[0];
        ESP_LOGW(TAG, "Current font size not in list, defaulting to: %s", currentFontSize.c_str());
    }
}

bool MainApp::isValidOption(const std::string& option, const std::vector<std::string>& validList) const {
    for (const auto& val : validList) {
        if (val == option) return true;
    }
    return false;
}

void MainApp::printAvailableLanguages() const {
    ESP_LOGI(TAG, "Available languages:");
    for (const auto& lang : availableLanguages) {
        ESP_LOGI(TAG, " - %s", lang.c_str());
    }
}

void MainApp::printAvailableFontSizes() const {
    ESP_LOGI(TAG, "Available font sizes:");
    for (const auto& size : availableFontSizes) {
        ESP_LOGI(TAG, " - %s", size.c_str());
    }
}

void MainApp::set_led1(int state) {
    gpio_set_level(LED1_PIN, state);
}

void MainApp::set_led2(int state) {
    gpio_set_level(LED2_PIN, state);
}

std::vector<std::string> MainApp::getWrappedSlice(const std::vector<std::string>& input, size_t startIndex, size_t count) const {
    std::vector<std::string> result;
    size_t n = input.size();
    if (n == 0 || count == 0) return result;

    startIndex = startIndex % n;  // normalize index

    for (size_t i = 0; i < count; ++i) {
        result.push_back(input[(startIndex + i) % n]);
    }
    return result;
}
