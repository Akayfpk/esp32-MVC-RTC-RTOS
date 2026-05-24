#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Model.h"


class Controller {
public:
  // Button indices
  enum ButtonIndex : uint8_t {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_SELECT1,
    BTN_SELECT2,
    BTN_COUNT
  };

private:
// Button pins (constexpr for compile-time)
  static constexpr uint8_t PIN_MAP[BTN_COUNT] = {
    32, // BTN_UP
    33, // BTN_DOWN
    25, // BTN_LEFT
    26, // BTN_RIGHT
    27, // BTN_SELECT1
    14  // BTN_SELECT2
  };
  // Debounce / repeat timing (ms)
  static constexpr uint32_t DEBOUNCE_DELAY_MS = 50;
  static constexpr uint32_t REPEAT_DELAY_MS = 200;
  
   struct ButtonConfig {
    uint8_t pin;
    bool lastState;
    uint32_t lastDebounceTime; // millis()
    bool pressed;
    uint32_t lastRepeatTime;

  ButtonConfig() : pin(0), lastState(true), lastDebounceTime(0), pressed(false), lastRepeatTime(0) {}
    void init(uint8_t p) { pin = p; lastState = digitalRead(p); lastDebounceTime = millis(); pressed = false; lastRepeatTime = 0; }
  };

   // Button states
  ButtonConfig m_buttons[BTN_COUNT];
  
  // Model reference
  Model* m_model;
  
 // Task handle
  TaskHandle_t m_taskHandle;
  volatile bool m_running;
  
// Private methods
  void initializeButtons();
  SystemEvent readButtons();
  bool isButtonPressed(uint8_t buttonIndex);
  void handleEvent(SystemEvent event);
  void handleMenuState(SystemEvent event);
  void handleSettingsState(SystemEvent event);
  void handleAboutState(SystemEvent event);
  void handleInputsState(SystemEvent event);
  void handleOutputsState(SystemEvent event);
  void handleConfirmExitState(SystemEvent event);
  
  // Static task wrapper
  static void taskWrapper(void* pvParameters);
  
  // Task implementation
  void buttonTask();

public:
  Controller();
  ~Controller();
  
  // Initialize controller
  bool initialize();
  
   // Start the controller task, allow custom stack and priority
  bool start(UBaseType_t priority = 1, uint32_t stackSize = 4096);
  
  // Stop the controller task
  void stop();
};

#endif // CONTROLLER_H