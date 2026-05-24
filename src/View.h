#ifndef VIEW_H
#define VIEW_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Model.h"

class View {
protected:
  Model* m_model;
  TaskHandle_t m_taskHandle;
  bool m_running;
  const char* m_taskName;
  uint32_t m_updateInterval; // in milliseconds
  
  // Static task wrapper - must be implemented by derived classes
  static void taskWrapper(void* pvParameters);
  
  // Virtual methods to be implemented by derived classes
  virtual bool initializeDisplay() = 0;
  virtual void renderDisplay() = 0;
  virtual void cleanup() {}
  
  // Task loop
  virtual void displayTask();

public:
  View(const char* taskName, uint32_t updateInterval = 250);
  virtual ~View();
  
  // Common interface
  virtual bool initialize();
  virtual bool start();
  virtual void stop();
  
  // Display state management
  virtual void renderMenuState();
  virtual void renderSettingsState();
  virtual void renderAboutState();
  virtual void renderInputsState();
  virtual void renderOutputsState();
  virtual void renderConfirmExitState();
};

#endif // VIEW_H