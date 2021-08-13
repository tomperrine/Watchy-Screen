#pragma once

#include <esp_event.h>

ESP_EVENT_DECLARE_BASE(WATCHY_EVENT_BASE);

typedef enum {
    WATCHY_EVENT = 0,
    WATCHY_EVENT_MENU_BTN_DOWN, 
    WATCHY_EVENT_BACK_BTN_DOWN, 
    WATCHY_EVENT_UP_BTN_DOWN, 
    WATCHY_EVENT_DOWN_BTN_DOWN, 
    WATCHY_EVENT_MAX
} WATCHY_EVENT_ID;

extern void start_event_handler(void);
extern void send_event(WATCHY_EVENT_ID eventID);