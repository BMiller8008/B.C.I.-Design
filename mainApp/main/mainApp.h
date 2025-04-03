#ifndef MAINAPP_H
#define MAINAPP_H

#include <string>
#include <vector>
#include "driver/gpio.h"
#include <esp_log.h>
#include "WiFiManager.h"
#include "OLED_Driver.h"
#include <format>

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
    int getMenuIdx() const;

    // Setters
    void setState(State newState);
    void setLanguage(const std::string& lang);
    void setFontSize(const std::string& size);
    void setAvailableLanguages(const std::vector<std::string>& langs);
    void setAvailableFontSizes(const std::vector<std::string>& sizes);
    void setMenuIdx(int idx);

    // Printing options
    void printAvailableLanguages() const;
    void printAvailableFontSizes() const;
    std::vector<std::string> getWrappedSlice(const std::vector<std::string>& input, size_t startIndex, size_t count) const;

    // OLED Control
    void displayLangChoice(OLED_Display* display);
    void displayFontChoice(OLED_Display* display);
    void displayMainMenu(OLED_Display* display);

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
    int menuIdx; //for keeping track of menu states
    float batt_charge; // for displaying battery charge

    static constexpr gpio_num_t LED1_PIN = GPIO_NUM_2;
    static constexpr gpio_num_t LED2_PIN = GPIO_NUM_4;

    static constexpr char* TAG = "MAIN";
    static constexpr int MENU_SIZE = 7;
};

#endif // MAINAPP_H
