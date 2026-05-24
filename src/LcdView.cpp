#include "LCDView.h"

/**
 * @brief Constructor - Initializes the LCD view with task name and stack size
 */
LCDView::LCDView() : View("LCD Task", 500), m_lcd(nullptr) {
}

/**
 * @brief Destructor - Ensures proper cleanup of LCD resources
 */
LCDView::~LCDView() {
  cleanup();
}

/**
 * @brief Initializes the LCD display hardware
 * @return true if initialization succeeded, false otherwise
 */
bool LCDView::initializeDisplay() {
  m_lcd = new SimpleLCD();
  
  // Initialize the LCD (SimpleLCD::begin() returns void)
  m_lcd->begin();
  
  // Verify LCD was properly allocated
  if (m_lcd == nullptr) {
    Serial.println("LCD allocation failed");
    return false;
  }
  
  // Display initial welcome message
  m_lcd->print("System Ready", 0, 0);
  m_lcd->printPadded("Loading...", 0, 1);
  
  Serial.println("LCD initialized successfully");
  return true;
}

/**
 * @brief Cleans up LCD resources
 */
void LCDView::cleanup() {
  if (m_lcd != nullptr) {
    m_lcd->clear();
    delete m_lcd;
    m_lcd = nullptr;
  }
}

/**
 * @brief FreeRTOS task wrapper function
 * @param pvParameters Pointer to the LCDView instance
 */
void LCDView::taskWrapper(void* pvParameters) {
  LCDView* view = static_cast<LCDView*>(pvParameters);
  view->displayTask();
}

/**
 * @brief Main display rendering function
 * Routes to appropriate state-specific renderer
 */
void LCDView::renderDisplay() {
  if (m_lcd == nullptr) return;
  
  // Get current system state from model
  SystemState currentState = m_model->getCurrentState();
  
  // Call appropriate renderer for current state
  switch (currentState) {
    case STATE_MENU:
      renderMenuState();
      break;
    case STATE_SETTINGS:
      renderSettingsState();
      break;
    case STATE_ABOUT:
      renderAboutState();
      break;
    case STATE_INPUTS:
      renderInputsState();
      break;
    case STATE_OUTPUTS:
      renderOutputsState();
      break;
    case STATE_CONFIRM_EXIT:
      renderConfirmExitState();
      break;
  }
}

// State-specific renderers
void LCDView::renderMenuState() {
  displayMenu();
}

void LCDView::renderSettingsState() {
  displaySettings();
}

void LCDView::renderAboutState() {
  displayAbout();
}

void LCDView::renderInputsState() {
  displayInputs();
}

void LCDView::renderOutputsState() {
  displayOutputs();
}

void LCDView::renderConfirmExitState() {
  displayConfirmExit();
}

/**
 * @brief Renders menu state
 * Shows menu position and current time
 */
void LCDView::displayMenu() {
    // Get model data
    const char* currentItem = m_model->getCurrentMenuItem();
    int currentIndex = m_model->getMenuIndex();
    
    // Format display lines
    char line1[21], line2[21];
    snprintf(line1, sizeof(line1), "Menu [%d/%d]", currentIndex + 1, m_model->getMenuLength());

    // Get and format time
   DateTime now = m_model->getTime(); 
    char timeStr[21];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
             now.hour(), now.minute(), now.second());

    snprintf(line2, sizeof(line2), "> %s %s", currentItem, timeStr);

    clearAndPrint(line1, line2);  // Atomic display update
}

/**
 * @brief Displays the settings state on LCD
 */
void LCDView::displaySettings() {
  clearAndPrint("Settings", "Configure System");
}

/**
 * @brief Displays the about state on LCD
 */
void LCDView::displayAbout() {
  clearAndPrint("About", "ESP32 Menu v1.0");
}

/**
 * @brief Displays the inputs state on LCD (placeholder)
 */
void LCDView::displayInputs() {
  clearAndPrint("Inputs", "SEL2/LEFT: Back");
}

/**
 * @brief Displays the outputs state on LCD (placeholder)
 */
void LCDView::displayOutputs() {
  clearAndPrint("Outputs", "SEL2/LEFT: Back");
}

/**
 * @brief Displays the exit confirmation state on LCD
 */
void LCDView::displayConfirmExit() {
  clearAndPrint("Exit System?", "SEL1:Yes SEL2:No");
}

/**
 * @brief Helper function to clear LCD and print two lines
 * @param line1 First line to display
 * @param line2 Second line to display (optional)
 */
void LCDView::clearAndPrint(const char* line1, const char* line2) {
  m_lcd->clear();
  m_lcd->printPadded(line1, 0, 0);
  if (line2 != nullptr) {
    m_lcd->printPadded(line2, 0, 1);
  }
}