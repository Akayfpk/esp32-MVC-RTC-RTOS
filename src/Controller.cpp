#include "Controller.h"
#include "RTClib.h"  

// Provide the out-of-class definition for the static constexpr array
constexpr uint8_t Controller::PIN_MAP[Controller::BTN_COUNT];
extern RTC_DS1307 rtc;
/**
 * @brief Constructor - Initializes controller with model reference
 */
Controller::Controller() : m_model(nullptr), m_taskHandle(nullptr) {
  m_model = Model::getInstance();
}

/**
 * @brief Destructor - Ensures proper task cleanup
 */
Controller::~Controller() {
  stop(); // Safely stop FreeRTOS task
}

/**
 * @brief Initializes the controller
 * @return true if initialization succeeded, false otherwise
 */
bool Controller::initialize() {
  if (m_model == nullptr) { 
    // Validate model instance
    Serial.println("Model not available");
    return false;
  }
  
  initializeButtons();
  // Hardware button setup
  Serial.println("Controller initialized");
  return true;
}

/**
 * @brief Configures button pins and initial state
 * Uses INPUT_PULLUP mode with active-low logic
 */
void Controller::initializeButtons() {
  // Configure all button pins as INPUT_PULLUP and initialize configs
  for (uint8_t i = 0; i < BTN_COUNT; ++i) {
    pinMode(PIN_MAP[i], INPUT_PULLUP);
    m_buttons[i].init(PIN_MAP[i]);
    // ensure known state
    m_buttons[i].lastState = digitalRead(m_buttons[i].pin);
    m_buttons[i].pressed = (m_buttons[i].lastState == LOW);
    m_buttons[i].lastDebounceTime = millis();
    m_buttons[i].lastRepeatTime = 0;
  }
}

/**
 * @brief Starts the controller task
 * @return true if task started successfully, false otherwise
 */
bool Controller::start(UBaseType_t priority, uint32_t stackSize) {
  if (m_taskHandle != nullptr) {
    Serial.println("Controller task already running");
    return false;
  }
  
    // Create FreeRTOS task for button handling
  // Set running after successful creation to avoid races
  BaseType_t result = xTaskCreate(
    taskWrapper,
    "ButtonTask",
    stackSize == 0 ? 2048 : stackSize, // fallback stack size
    this, // Task parameter
    priority == 0 ? 2 : priority,    // fallback priority
    &m_taskHandle
  );

  if (result != pdPASS) {
    Serial.println("Failed to create ButtonTask");
    m_taskHandle = nullptr;
    m_running = false;
    return false;
  }

  m_running = true;
  return true;
}


/**
 * @brief Stops the controller task (graceful)
 */
void Controller::stop() {
  // Request graceful stop
  m_running = false;

  // Wait briefly for task to exit and clear m_taskHandle
  const TickType_t waitTicks = pdMS_TO_TICKS(200);
  TickType_t start = xTaskGetTickCount();
  while (m_taskHandle != nullptr && (xTaskGetTickCount() - start) < waitTicks) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  // If still alive after timeout, force-delete (last resort)
  if (m_taskHandle != nullptr) {
    Serial.println("Forcing Controller task delete");
    vTaskDelete(m_taskHandle);
    m_taskHandle = nullptr;
  }
}


/**
 * @brief FreeRTOS task wrapper function
 * @param pvParameters Pointer to the Controller instance
 */
void Controller::taskWrapper(void* pvParameters) {
  Controller* controller = static_cast<Controller*>(pvParameters);
  controller->buttonTask();
}

/**
 * @brief Main button task function
 * Continuously polls buttons and handles events
 */
void Controller::buttonTask() {
  Serial.println("Button task started");
  
  while (m_running) {
    SystemEvent event = readButtons();
    
    if (event != EVENT_NONE) {
      handleEvent(event);
    }
    
    vTaskDelay(pdMS_TO_TICKS(10)); // 10ms polling rate
  }

  // clear handle and delete self from task context
  m_taskHandle = nullptr;
  vTaskDelete(nullptr);
}

/**
 * @brief Reads and debounces all buttons
 * @return SystemEvent representing the button press (or EVENT_NONE)
 */
SystemEvent Controller::readButtons() {
  unsigned long currentTime = millis();
  
  for (uint8_t i = 0; i < BTN_COUNT; ++i) {
    bool currentState = digitalRead(m_buttons[i].pin);
    
    // Check for state change
    if (currentState != m_buttons[i].lastState) {
      m_buttons[i].lastDebounceTime = currentTime;
    }
    
    // Check if debounce period has passed
    if ((currentTime - m_buttons[i].lastDebounceTime) > DEBOUNCE_DELAY_MS) {
      // Detect button press (LOW for pull-up)
      if (!m_buttons[i].pressed && currentState == LOW) {
        m_buttons[i].pressed = true;
        m_buttons[i].lastState = currentState;
        
        // Return corresponding event
        switch (i) {
          case BTN_UP:     return EVENT_UP;
          case BTN_DOWN:   return EVENT_DOWN;
          case BTN_LEFT:   return EVENT_LEFT;
          case BTN_RIGHT:  return EVENT_RIGHT;
          case BTN_SELECT1:return EVENT_SELECT1;
          case BTN_SELECT2:return EVENT_SELECT2;
          default: break;
        }
      }
      // Detect button release
      else if (m_buttons[i].pressed && currentState == HIGH) {
        m_buttons[i].pressed = false;
      }
    }
    
    m_buttons[i].lastState = currentState;
  }
  
  return EVENT_NONE;
}


/**
 * @brief Handles system events based on current state
 * @param event The system event to handle
 */
void Controller::handleEvent(SystemEvent event) {
  SystemState currentState = m_model->getCurrentState();
  
  // Route to appropriate state handler
  switch (currentState) {
    case STATE_MENU:
      handleMenuState(event);
      break;
    case STATE_SETTINGS:
      handleSettingsState(event);
      break;
    case STATE_ABOUT:
      handleAboutState(event);
      break;
    case STATE_CONFIRM_EXIT:
      handleConfirmExitState(event);
      break;
    default:
      break;
  }
}
// State-specific event handlers
void Controller::handleMenuState(SystemEvent event) {
  switch (event) {
    case EVENT_UP:
      m_model->decrementMenuIndex();
      break;
    case EVENT_DOWN:
      m_model->incrementMenuIndex();
      break;
    case EVENT_SELECT1:
      {
        int menuIndex = m_model->getMenuIndex();
        switch (menuIndex) {
          case 0: { // Home
            DateTime now = rtc.now();
            char timeStr[25];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d %02d/%02d/%04d",
              now.hour(), now.minute(), now.second(),
              now.day(), now.month(), now.year());

            Serial.print("Home selected - Current Time: ");
            Serial.println(timeStr);

            m_model->setCurrentTime(now);
            break;
          }
          case 1: // Settings
            m_model->setState(STATE_SETTINGS);
            break;
          case 2: // About
            m_model->setState(STATE_ABOUT);
            break;
          case 3: // Exit
            m_model->setState(STATE_CONFIRM_EXIT);
            break;
        }
      }
      break;
    case EVENT_SELECT2:
      Serial.println("Secondary select in menu");
      break;
    default:
      break;
  }
}

void Controller::handleSettingsState(SystemEvent event) {
  switch (event) {
    case EVENT_LEFT:
    case EVENT_SELECT2:
      m_model->setState(STATE_MENU);
      Serial.println("Returning to menu from settings");
      break;
    case EVENT_SELECT1:
      Serial.println("Settings action");
      break;
    default:
      break;
  }
}

void Controller::handleAboutState(SystemEvent event) {
  switch (event) {
    case EVENT_LEFT:
    case EVENT_SELECT2:
      m_model->setState(STATE_MENU);
      Serial.println("Returning to menu from about");
      break;
    default:
      break;
  }
}

void Controller::handleConfirmExitState(SystemEvent event) {
  switch (event) {
    case EVENT_SELECT1: // Confirm exit
      Serial.println("Exit confirmed - implement shutdown logic");
      m_model->setState(STATE_MENU); // Temporary - would normally shutdown
      break;
    case EVENT_LEFT:
    case EVENT_SELECT2: // Cancel exit
      m_model->setState(STATE_MENU);
      Serial.println("Exit cancelled");
      break;
    default:
      break;
  }
}
