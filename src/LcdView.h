#ifndef LCDVIEW_H
#define LCDVIEW_H

#include "View.h"
#include "SimpleLCD.h"

class LCDView : public View {
private:
  SimpleLCD* m_lcd;
  
  // Display helper methods
  void displayMenu();
  void displaySettings();
  void displayAbout();
  void displayInputs();
  void displayOutputs();
  void displayConfirmExit();
  void clearAndPrint(const char* line1, const char* line2 = nullptr);

protected:
  // Override virtual methods from View
  bool initializeDisplay() override;
  void renderDisplay() override;
  void cleanup() override;
  
  // Static task wrapper for this specific view
  static void taskWrapper(void* pvParameters);

public:
  LCDView();
  virtual ~LCDView();
  
  // Override display state methods
  void renderMenuState() override;
  void renderSettingsState() override;
  void renderAboutState() override;
  void renderInputsState() override;
  void renderOutputsState() override;
  void renderConfirmExitState() override;
};

#endif // LCDVIEW_H