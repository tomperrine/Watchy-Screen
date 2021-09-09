#pragma once

#include <esp_event.h>

namespace Watchy_Event {

ESP_EVENT_DECLARE_BASE(BASE);

typedef enum {
  NULL_EVENT = 0,
  MENU_BTN_DOWN,
  BACK_BTN_DOWN,
  UP_BTN_DOWN,
  DOWN_BTN_DOWN,
  MAX
} ID;

extern esp_event_loop_handle_t loop;

extern TaskHandle_t producerTask;

extern void start(void);
extern void send(ID eventID);
}  // namespace Watchy_Events