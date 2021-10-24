#pragma once

#include <vector>

#include <esp_event.h>
#include "GetLocation.h"

namespace Watchy_Event {

ESP_EVENT_DECLARE_BASE(BASE);

extern esp_event_loop_handle_t eventLoop;

typedef enum {
  NULL_EVENT = 0,
  MENU_BTN_DOWN,
  BACK_BTN_DOWN,
  UP_BTN_DOWN,
  DOWN_BTN_DOWN,
  UPDATE_SCREEN,
  LOCATION_UPDATE,
  TIME_SYNC,
  MAX
} ID;

class Event {
  public:
  ID id;
  uint64_t micros;
  union {
    Watchy_GetLocation::location loc; // LOCATION_UPDATE
    timeval tv; // TIME_SYNC
  } data;
  void send();
};

extern TaskHandle_t producerTask; // so interrupt handlers know who to notify

class BackgroundTask {
 private:
  static std::vector<const BackgroundTask *> tasks;

  const char *name;
  const TaskFunction_t taskFunction;
  TaskHandle_t task;

 public:
  BackgroundTask(const char *n, const TaskFunction_t t)
      : name(n), taskFunction(t), task(nullptr){};
  const char *Name() const { return name; }
  void begin();
  void kill();
  ~BackgroundTask();

  void add() const;
  void remove() const;
  static bool running();
};

extern void setUpdateInterval(uint32_t ms);
extern void enableUpdateTimer();

extern void start(void);
}  // namespace Watchy_Events