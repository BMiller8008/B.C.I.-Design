#ifndef MAINAPP_H
#define MAINAPP_H

#include <string>
#include <vector>
#include "driver/gpio.h"
#include <esp_log.h>

class MainApp {
public:
    enum class State { OFF, ON };

    MainApp();

    // Getters
    State getState() const;
    std::string getLanguage() const;
    std::string getFontSize() const;
    std::vector<std::string> getAvailableLanguages() const;
    std::vector<std::string> getAvailableFontSizes() const;

    // Setters
    void setState(State newState);
    void setLanguage(const std::string& lang);
    void setFontSize(const std::string& size);
    void setAvailableLanguages(const std::vector<std::string>& langs);
    void setAvailableFontSizes(const std::vector<std::string>& sizes);

    // Printing options
    void printAvailableLanguages() const;
    void printAvailableFontSizes() const;
    std::vector<std::string> getWrappedSlice(const std::vector<std::string>& input, size_t startIndex, size_t count) const;



    // LED control
    void set_led1(int state);
    void set_led2(int state);

private:
    void initLeds();
    bool isValidOption(const std::string& option, const std::vector<std::string>& validList) const;

    State currentState;
    std::string currentLanguage;
    std::string currentFontSize;
    std::vector<std::string> availableLanguages;
    std::vector<std::string> availableFontSizes;

    static constexpr gpio_num_t LED1_PIN = GPIO_NUM_2;
    static constexpr gpio_num_t LED2_PIN = GPIO_NUM_4;

    static constexpr char* TAG = "MAIN";
};

#endif // MAINAPP_H
