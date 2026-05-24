#include "Model.h"


// Initialize static members
Model* Model::m_instance = nullptr;
const char* Model::m_menuItems[] = { "Home", "Settings", "About", "Inputs", "Outputs", "Exit" };
const int Model::m_menuLength = sizeof(Model::m_menuItems) / sizeof(Model::m_menuItems[0]);

Model::Model() 
  : m_menuIndex(0),
    m_currentState(STATE_MENU),
    m_stateChanged(false),
    m_stateMutex(nullptr),
    m_displayMutex(nullptr),
    m_timeMutex(nullptr) {
}

Model* Model::getInstance() {
  if (m_instance == nullptr) {
    m_instance = new Model();
  }
  return m_instance;
}

bool Model::initialize() {
  // Create synchronization primitives
  m_stateMutex = xSemaphoreCreateMutex();
  m_displayMutex = xSemaphoreCreateMutex();
  m_timeMutex = xSemaphoreCreateMutex();

  if (!m_stateMutex || !m_displayMutex || !m_timeMutex) {
    Serial.println("Failed to create mutexes");
    return false;
  }

  // Initialize RTC hardware
  if (m_rtc.begin()) {
    m_rtcAvailable = true;
    if (xSemaphoreTake(m_timeMutex, pdMS_TO_TICKS(100))) {
      m_currentTime = m_rtc.now();
      xSemaphoreGive(m_timeMutex);
    }
    Serial.println("RTC initialized successfully");
  } else {
    m_rtcAvailable = false;
    Serial.println("RTC not available - using system time");
  }

  return true;
}

void Model::updateTime() {
  if (m_rtcAvailable && xSemaphoreTake(m_timeMutex, pdMS_TO_TICKS(100))) {
    m_currentTime = m_rtc.now();
    xSemaphoreGive(m_timeMutex);
  }
  // Else maintain existing time (no RTC fallback implementation)
}

DateTime Model::getTime() {
  DateTime copy;
  if (xSemaphoreTake(m_timeMutex, portMAX_DELAY)) {
    copy = m_currentTime;
    xSemaphoreGive(m_timeMutex);
  }
  return copy;
}

String Model::getFormattedTime() {
  DateTime now = getTime();
  char buf[24];
  snprintf(buf, sizeof(buf), "%02u:%02u:%02u %02u/%02u/%04u",
           now.hour(), now.minute(), now.second(),
           now.day(), now.month(), now.year());
  return String(buf);
}

String Model::getCurrentTimeString() const {
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
           m_currentTime.hour(), m_currentTime.minute(), m_currentTime.second());
  return String(buf);
}

/**
 * @brief Gets the current menu index
 * @return Current menu index (0-based)
 */
int Model::getMenuIndex() {
  int index = 0;
  // Protect access with mutex
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(10))) {
    index = m_menuIndex;
    xSemaphoreGive(m_stateMutex);
  }
  return index;
}

/**
 * @brief Sets the menu index
 * @param index New menu index (will be clamped to valid range)
 */
void Model::setMenuIndex(int index) {
  // Protect access with mutex
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(100))) {
    // Validate index range
    if (index >= 0 && index < m_menuLength) {
      m_menuIndex = index;
      m_stateChanged = true;  // Mark state as changed
    }
    xSemaphoreGive(m_stateMutex);
  }
}

/**
 * @brief Increments menu index with wrap-around
 */
void Model::incrementMenuIndex() {
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(100))) {
    // Circular increment
    m_menuIndex = (m_menuIndex + 1) % m_menuLength;
    m_stateChanged = true;
    xSemaphoreGive(m_stateMutex);
  }
}

/**
 * @brief Decrements menu index with wrap-around
 */
void Model::decrementMenuIndex() {
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(100))) {
    // Circular decrement (with positive modulo)
    m_menuIndex = (m_menuIndex - 1 + m_menuLength) % m_menuLength;
    m_stateChanged = true;
    xSemaphoreGive(m_stateMutex);
  }
}

/**
 * @brief Gets the current system state
 * @return Current system state
 */
SystemState Model::getCurrentState() {
  SystemState state = STATE_MENU;  // Default fallback
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(10))) {
    state = m_currentState;
    xSemaphoreGive(m_stateMutex);
  }
  return state;
}

/**
 * @brief Sets a new system state
 * @param newState State to transition to
 */
void Model::setState(SystemState newState) {
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(10))) {
    // Only update if state actually changed
    if (m_currentState != newState) {
      m_currentState = newState;
      m_stateChanged = true;
      Serial.print("State changed to: ");
      Serial.println(newState);
    }
    xSemaphoreGive(m_stateMutex);
  }
}

/**
 * @brief Checks if state has changed since last check
 * @return true if state changed, false otherwise
 */
bool Model::hasStateChanged() {
  bool changed = false;
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(20))) {
    changed = m_stateChanged;
    xSemaphoreGive(m_stateMutex);
  }
  return changed;
}

/**
 * @brief Clears the state changed flag
 */
void Model::clearStateChanged() {
  if (xSemaphoreTake(m_stateMutex, pdMS_TO_TICKS(10))) {
    m_stateChanged = false;
    xSemaphoreGive(m_stateMutex);
  }
}

/**
 * @brief Gets the current menu item text
 * @return Pointer to menu item string
 */
const char* Model::getCurrentMenuItem() {
  return getMenuItem(getMenuIndex());
}

/**
 * @brief Gets menu item text by index
 * @param index Menu item index (0-based)
 * @return Pointer to menu item string or "Invalid" if out of range
 */
const char* Model::getMenuItem(int index) {
  // Validate index range
  if (index >= 0 && index < m_menuLength) {
    return m_menuItems[index];
  }
  return "Invalid";
}

/**
 * @brief Gets total number of menu items
 * @return Menu length
 */
int Model::getMenuLength() {
  return m_menuLength;
}

/**
 * @brief Acquires display mutex for thread-safe display operations
 * @param timeout Maximum time to wait for mutex
 * @return true if mutex acquired, false otherwise
 */
bool Model::acquireDisplayMutex(TickType_t timeout) {
  return xSemaphoreTake(m_displayMutex, timeout) == pdTRUE;
}

/**
 * @brief Releases display mutex
 */
void Model::releaseDisplayMutex() {
  xSemaphoreGive(m_displayMutex);
}

/**
 * @brief Cleans up model resources
 */
void Model::cleanup() {
  if (m_stateMutex) vSemaphoreDelete(m_stateMutex);
  if (m_displayMutex) vSemaphoreDelete(m_displayMutex);
  if (m_timeMutex) vSemaphoreDelete(m_timeMutex);
  m_stateMutex = m_displayMutex = m_timeMutex = nullptr;
}