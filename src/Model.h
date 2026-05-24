#ifndef MODEL_H
#define MODEL_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "RTClib.h"

// System state machine states
enum SystemState {
  STATE_MENU,
  STATE_SETTINGS,
  STATE_ABOUT,
  STATE_INPUTS,
  STATE_OUTPUTS,
  STATE_CONFIRM_EXIT
};

// System event types for state transitions
enum SystemEvent {
  EVENT_UP,
  EVENT_DOWN,
  EVENT_LEFT,
  EVENT_RIGHT,
  EVENT_SELECT1,
  EVENT_SELECT2,
  EVENT_TIMEOUT,
  EVENT_NONE
};

class Model {
private:
  // Singleton instance
  static Model* m_instance;  // <-- This is the crucial declaration
  
  // Menu navigation state
  volatile int m_menuIndex;
  volatile SystemState m_currentState;
  volatile bool m_stateChanged;
  
  // Menu configuration
  static const char* m_menuItems[];
  static const int m_menuLength;
  
  // Thread synchronization
  SemaphoreHandle_t m_stateMutex;
  SemaphoreHandle_t m_displayMutex;
  
  // RTC and time management
  RTC_DS1307 m_rtc;
  DateTime m_currentTime;
  SemaphoreHandle_t m_timeMutex;
  bool m_rtcAvailable = false;

  // Private constructor for singleton
  Model();

public:
  // Singleton access
  static Model* getInstance();
  
  // Initialization and cleanup
  bool initialize();
  void cleanup();

  // RTC status
  bool isRTCAvailable() const { return m_rtcAvailable; }

  // Time management
  void updateTime();
  DateTime getTime();
  String getFormattedTime();
  String getCurrentTimeString() const;
  void setCurrentTime(const DateTime& dt) { m_currentTime = dt; }

  // Menu operations
  int getMenuIndex();
  void setMenuIndex(int index);
  void incrementMenuIndex();
  void decrementMenuIndex();
  
  // State operations
  SystemState getCurrentState();
  void setState(SystemState newState);
  bool hasStateChanged();
  void clearStateChanged();
  
  // Menu data access
  const char* getCurrentMenuItem();
  const char* getMenuItem(int index);
  int getMenuLength();
  
  // Display synchronization
  bool acquireDisplayMutex(TickType_t timeout = portMAX_DELAY);
  void releaseDisplayMutex();
};

#endif // MODEL_H