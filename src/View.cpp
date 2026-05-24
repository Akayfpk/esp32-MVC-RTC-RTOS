#include "View.h"

View::View(const char* taskName, uint32_t updateInterval)
  : m_model(nullptr), m_taskHandle(nullptr), m_running(false),
    m_taskName(taskName), m_updateInterval(updateInterval) {
  m_model = Model::getInstance();
}

View::~View() {
  stop();
}

bool View::initialize() {
  if (m_model == nullptr) {
    Serial.println("Model not available for view");
    return false;
  }
  
  if (!initializeDisplay()) {
    Serial.print("Failed to initialize display for ");
    Serial.println(m_taskName);
    return false;
  }
  
  Serial.print(m_taskName);
  Serial.println(" initialized successfully");
  return true;
}

bool View::start() {
  if (m_taskHandle != nullptr) {
    Serial.print(m_taskName);
    Serial.println(" task already running");
    return false;
  }
  
  m_running = true;
  
  BaseType_t result = xTaskCreate(
    taskWrapper,
    m_taskName,
    2048,
    this,
    1, // Lower priority than controller
    &m_taskHandle
  );
  
  return result == pdPASS;
}

void View::stop() {
  m_running = false;
  
  if (m_taskHandle != nullptr) {
    vTaskDelete(m_taskHandle);
    m_taskHandle = nullptr;
  }
  
  cleanup();
}

void View::taskWrapper(void* pvParameters) {
  View* view = static_cast<View*>(pvParameters);
  view->displayTask();
}

void View::displayTask() {
  Serial.print(m_taskName);
  Serial.println(" task started");
  
  SystemState lastState = STATE_MENU;
  bool forceUpdate = true;
  
  while (m_running) {
    SystemState currentState = m_model->getCurrentState();
    bool stateChanged = m_model->hasStateChanged();
    
    // Update display if state changed or forced
    if (stateChanged || forceUpdate || currentState != lastState) {
      if (m_model->acquireDisplayMutex(pdMS_TO_TICKS(100))) {
        renderDisplay();
        m_model->releaseDisplayMutex();
        
        if (stateChanged) {
          m_model->clearStateChanged();
        }
        
        lastState = currentState;
        forceUpdate = false;
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(m_updateInterval));
  }
}

void View::renderMenuState() {
  // Default implementation - to be overridden
  Serial.println("Default menu render");
}

void View::renderSettingsState() {
  // Default implementation - to be overridden
  Serial.println("Default settings render");
}

void View::renderAboutState() {
  // Default implementation - to be overridden
  Serial.println("Default about render");
}

void View::renderInputsState() {
  // Default implementation - to be overridden
  Serial.println("Default inputs render");
}

void View::renderOutputsState() {
  // Default implementation - to be overridden
  Serial.println("Default outputs render");
}

void View::renderConfirmExitState() {
  // Default implementation - to be overridden
  Serial.println("Default confirm exit render");
}