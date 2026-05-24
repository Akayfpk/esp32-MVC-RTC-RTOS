#include "OLEDView.h"

/**
 * @brief Constructor - Initializes the OLED display view
 * @param name Task name
 * @param stackSize Task stack size in bytes
 */
OLEDView::OLEDView() : View("OLED Task", 250), m_oled(nullptr) {
}

/**
 * @brief Destructor - Cleans up display resources
 */
OLEDView::~OLEDView() {
  cleanup();
}

/**
 * @brief Initializes the OLED display hardware
 * @return true if initialization succeeded, false otherwise
 */
bool OLEDView::initializeDisplay() {
  // Create new SSD1306 display instance
  m_oled = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  
  // Attempt to initialize the display
  if (!m_oled->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED allocation failed");
    delete m_oled;
    m_oled = nullptr;
    return false;
  }
  
  // Initial display setup
  m_oled->clearDisplay();
  m_oled->setTextSize(1);              // Normal 1:1 pixel scale
  m_oled->setTextColor(SSD1306_WHITE); // Draw white text
  m_oled->setCursor(0, 0);             // Start at top-left corner
  m_oled->println("System Starting...");
  m_oled->display();                   // Show initial message
  
  return true;
}

/**
 * @brief Cleans up display resources
 */
void OLEDView::cleanup() {
  if (m_oled != nullptr) {
    // Clear display before shutting down
    m_oled->clearDisplay();
    m_oled->display();
    delete m_oled;
    m_oled = nullptr;
  }
}

/**
 * @brief FreeRTOS task wrapper function
 * @param pvParameters Pointer to the OLEDView instance
 */
void OLEDView::taskWrapper(void* pvParameters) {
  OLEDView* view = static_cast<OLEDView*>(pvParameters);
  view->displayTask();
}

/**
 * @brief Main display rendering function
 * Calls appropriate state-specific renderer based on current model state
 */
void OLEDView::renderDisplay() {
  if (m_oled == nullptr) return;
  
  // Get current system state from model
  SystemState currentState = m_model->getCurrentState();
  
  // Render appropriate view based on state
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
    case STATE_CONFIRM_EXIT:
      renderConfirmExitState();
      break;
  }
}

// State-specific renderers
void OLEDView::renderMenuState() {
  drawMenu();
}

void OLEDView::renderSettingsState() {
  drawSettings();
}

void OLEDView::renderAboutState() {
  drawAbout();
}

void OLEDView::renderConfirmExitState() {
  drawConfirmExit();
}

/**
 * @brief Renders the main menu view
 *
 * Uses a scroll window of MENU_VISIBLE_ROWS items so the menu can hold
 * an arbitrary number of entries without overflowing the panel. The
 * window slides so the selected index is always visible. Up/down arrow
 * glyphs appear when items exist outside the window.
 */
void OLEDView::drawMenu() {
  m_oled->clearDisplay();
  drawHeader("Main Menu");

  const int menuLength   = m_model->getMenuLength();
  const int currentIndex = m_model->getMenuIndex();
  const int visible      = (menuLength < MENU_VISIBLE_ROWS) ? menuLength : MENU_VISIBLE_ROWS;

  // Slide the window so currentIndex stays inside [first, first+visible)
  int first = currentIndex - (visible / 2);
  if (first < 0) first = 0;
  if (first > menuLength - visible) first = menuLength - visible;

  for (int row = 0; row < visible; row++) {
    int idx = first + row;
    drawMenuItem(idx, row, idx == currentIndex);
  }

  // Scroll indicators (top/bottom carets) when more items exist off-window
  if (first > 0) {
    m_oled->setTextColor(SSD1306_WHITE);
    m_oled->setCursor(SCREEN_WIDTH - 6, 13);
    m_oled->print('^');
  }
  if (first + visible < menuLength) {
    m_oled->setTextColor(SSD1306_WHITE);
    m_oled->setCursor(SCREEN_WIDTH - 6, 13 + (visible - 1) * 10);
    m_oled->print('v');
  }

  // Navigation hint at bottom
  m_oled->setCursor(0, SCREEN_HEIGHT - 8);
  m_oled->setTextSize(1);
  m_oled->print("UP/DOWN  SEL:Choose");

  m_oled->display();
}

/**
 * @brief Renders the settings view
 */
void OLEDView::drawSettings() {
  m_oled->clearDisplay();
  drawHeader("Settings");
  
  // Draw settings content
  m_oled->setCursor(0, 20);
  m_oled->setTextSize(1);
  m_oled->println("System Configuration");
  m_oled->println("");
  m_oled->println("Version: 1.0.0");
  m_oled->println("FreeRTOS: Active");
  m_oled->println("Display: OLED + LCD");
  
  // Add navigation hint
  m_oled->setCursor(0, SCREEN_HEIGHT - 8);
  m_oled->print("LEFT/SELECT2: Back");
  
  m_oled->display();
}

/**
 * @brief Renders the about view
 */
void OLEDView::drawAbout() {
  m_oled->clearDisplay();
  drawHeader("About");
  
  // Draw about content
  m_oled->setCursor(0, 20);
  m_oled->setTextSize(1);
  m_oled->println("ESP32 Menu System");
  m_oled->println("MVC Architecture");
  m_oled->println("FreeRTOS Tasks");
  m_oled->println("");
  m_oled->println("Dual Display Support");
  
  // Add navigation hint
  m_oled->setCursor(0, SCREEN_HEIGHT - 8);
  m_oled->print("LEFT/SELECT2: Back");
  
  m_oled->display();
}

/**
 * @brief Renders the exit confirmation view
 */
void OLEDView::drawConfirmExit() {
  m_oled->clearDisplay();
  drawHeader("Confirm Exit");
  
  // Draw confirmation prompt
  m_oled->setCursor(0, 25);
  m_oled->setTextSize(2);
  m_oled->println("EXIT?");
  
  // Draw options
  m_oled->setTextSize(1);
  m_oled->setCursor(0, 45);
  m_oled->println("SELECT1: Yes");
  m_oled->println("LEFT/SELECT2: No");
  
  m_oled->display();
}

/**
 * @brief Draws a standard header for views
 * @param title Header text to display
 */
void OLEDView::drawHeader(const char* title) {
  m_oled->setTextSize(1);
  m_oled->setCursor(0, 0);
  m_oled->print(title);
  
  // Draw underline below header
  for (int x = 0; x < SCREEN_WIDTH; x++) {
    m_oled->drawPixel(x, 10, SSD1306_WHITE);
  }
}

/**
 * @brief Draws a single menu item at a visible-row slot
 * @param index    Model item index (used to fetch the label)
 * @param row      Visible-row position (0..MENU_VISIBLE_ROWS-1)
 * @param selected Whether this item is currently selected
 */
void OLEDView::drawMenuItem(int index, int row, bool selected) {
  int y = 15 + (row * 10);  // Y from window-relative row, not absolute index

  m_oled->setTextSize(1);
  m_oled->setCursor(0, y);

  if (selected) {
    m_oled->print("> ");  // Selection indicator
    // Highlight background for selected item
    m_oled->fillRect(15, y, 80, 8, SSD1306_WHITE);
    m_oled->setTextColor(SSD1306_BLACK);  // Black text on white background
  } else {
    m_oled->print("  ");  // Empty space for unselected items
    m_oled->setTextColor(SSD1306_WHITE);  // White text on black background
  }

  // Draw the menu item text
  m_oled->print(m_model->getMenuItem(index));

  // Reset text color to default
  m_oled->setTextColor(SSD1306_WHITE);
}