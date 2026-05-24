#ifndef OLEDVIEW_H
#define OLEDVIEW_H

#include "View.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>

class OLEDView : public View {
private:
  // OLED configuration
  static const int SCREEN_WIDTH = 128;
  static const int SCREEN_HEIGHT = 64;
  static const int OLED_ADDR = 0x3C;
  static const int OLED_RESET = -1;
  
  Adafruit_SSD1306* m_oled;
  
  // Display helper methods
  void drawMenu();
  void drawSettings();
  void drawAbout();
  void drawInputs();
  void drawOutputs();
  void drawConfirmExit();
  void drawHeader(const char* title);
  void drawMenuItem(int index, int row, bool selected);

  // Number of menu rows visible at once in the scroll window
  static const int MENU_VISIBLE_ROWS = 4;

protected:
  // Override virtual methods from View
  bool initializeDisplay() override;
  void renderDisplay() override;
  void cleanup() override;
  
  // Static task wrapper for this specific view
  static void taskWrapper(void* pvParameters);

public:
  OLEDView();
  virtual ~OLEDView();
  
  // Override display state methods
  void renderMenuState() override;
  void renderSettingsState() override;
  void renderAboutState() override;
  void renderInputsState() override;
  void renderOutputsState() override;
  void renderConfirmExitState() override;
};

#endif // OLEDVIEW_H