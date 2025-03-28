#include <stdio.h>
#include "mainApp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void) {
    MainApp app;

    printf("Initial state: %s\n", app.getState() == MainApp::State::ON ? "ON" : "OFF");
    printf("Current language: %s\n", app.getLanguage().c_str());
    printf("Current font size: %s\n", app.getFontSize().c_str());

    app.printAvailableLanguages();
    app.printAvailableFontSizes();

    // Change language and font size
    app.setLanguage("French");
    app.setFontSize("12");
    printf("\nUpdated language: %s\n", app.getLanguage().c_str());
    printf("Updated font size: %s\n", app.getFontSize().c_str());

    // Toggle LEDs
    printf("Turning on LED1 and LED2...\n");
    app.set_led1(1);
    app.set_led2(1);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Turning off LED1 and LED2...\n");
    app.set_led1(0);
    app.set_led2(0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("Done.\n");

    std::vector<std::string> langSlice = app.getWrappedSlice(app.getAvailableLanguages(), 5, 3);
    printf("Wrapped language slice:\n");
    for (const auto& lang : langSlice) {
        printf(" - %s\n", lang.c_str());
    }

}
