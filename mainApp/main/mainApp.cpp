#include "mainApp.h"
#include <iostream>
#include <stdexcept>

MainApp::MainApp()
    : currentState(State::ON),
      currentLanguage("English"),
      currentFontSize("12")
{
    availableLanguages = {"English", "Spanish", "Hindi", "Mandarin", "Arabic", "German"};
    availableFontSizes = {"8", "12", "16"};
    batt_charge = 0.0;
    menuIdx = 0;
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

void MainApp::setMenuIdx(int idx) {
    menuIdx = idx;
}

int MainApp::getMenuIdx() const {
    return menuIdx;
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

void MainApp::displayLangChoice(OLED_Display* display) {
    // clearing buffer
    display->clear_buffer();
    // getting options based on current menu pointer
    std::vector<std::string> options = availableLanguages;

    // Drawing Each Option to the Screen
    for (size_t i = 0; i < options.size(); i++)
    {
        std::string msg = options[i];
        // showing choice
        if (i == (menuIdx % options.size()))
        {
            msg.insert(0, ">");
        }
        else 
        {
            msg.insert(0, " ");
        }
        
        display->drawText(10, 10 * i, msg.c_str(), &Font8, BLACK, WHITE);
    }
    // updating display
    display->display();
}

void MainApp::displayFontChoice(OLED_Display* display) {
    // clearing buffer
    display->clear_buffer();
    // getting options based on current menu pointer
    std::vector<std::string> options = availableFontSizes;

    // Drawing Each Option to the Screen
    for (size_t i = 0; i < options.size(); i++)
    {
        std::string msg = options[i];
        // showing choice
        if (i == (menuIdx % options.size()))
        {
            msg.insert(0, ">");
        }
        else 
        {
            msg.insert(0, " ");
        }
        
        display->drawText(10, 10 * i, msg.c_str(), &Font8, BLACK, WHITE);
    }
    // updating display
    display->display();
}

void MainApp::displayMainMenu(OLED_Display* display) {
    // clearing buffer
    display->clear_buffer();
    std::vector<std::string> options = {"Languages", "Fonts"};

    // Displaying Battery Charge
    std::string bat_str = std::format("{:.0f}", batt_charge);
    bat_str += "%";
    display->drawText(100, 10, bat_str.c_str(), &Font12, BLACK, WHITE);

    // Displaying options
    for (size_t i = 0; i < options.size(); i++)
    {
        std::string msg = options[i];
        // showing choice
        if (i == (menuIdx % options.size()))
        {
            msg.insert(0, ">");
        }
        else 
        {
            msg.insert(0, " ");
        }
        
        display->drawText(20, 10 * (i + 3), msg.c_str(), &Font12, BLACK, WHITE);
    }
    // updating display
    display->display();
}

void MainApp::updateBattery() {
    // reading voltage
    int raw_value = adc1_get_raw(ADC1_CHANNEL_0);
    float voltage = (raw_value / 4095.0) * 3.3;

    // Getting status
    if (voltage >= 2.65) {
        batt_charge = 100.0;
    } else if (voltage >= 2.53) {
        batt_charge = 75.0;
    } else if (voltage >= 2.33) {
        batt_charge = 50.0;
    } else if (voltage >= 1.5) {
        batt_charge = 25.0;
    } else {
        batt_charge = 0.0;
    }
}