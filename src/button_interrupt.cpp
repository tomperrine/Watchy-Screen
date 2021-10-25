#include "button_interrupt.h"

#include <freertos/queue.h>

#include "Events.h"

#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 150
#endif

static void IRAM_ATTR ISR_Send(const Watchy_Event::Event e) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendToBackFromISR(Watchy_Event::q, (const void *)&e,
                          &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void IRAM_ATTR ISR_MenuButtonPress() {
  ISR_Send(Watchy_Event::Event{.id = Watchy_Event::MENU_BTN_DOWN});
}

void IRAM_ATTR ISR_BackButtonPress() {
  ISR_Send(Watchy_Event::Event{.id = Watchy_Event::BACK_BTN_DOWN});
}

void IRAM_ATTR ISR_UpButtonPress() {
  ISR_Send(Watchy_Event::Event{.id = Watchy_Event::UP_BTN_DOWN});
}

void IRAM_ATTR ISR_DownButtonPress() {
  ISR_Send(Watchy_Event::Event{.id = Watchy_Event::DOWN_BTN_DOWN});
}

void buttonSetup(int pin, ButtonIndex index) {
  pinMode(pin, INPUT_PULLUP);
  switch (index) {
    case menu_btn:
      attachInterrupt(pin, ISR_MenuButtonPress, 2);
      break;
    case back_btn:
      attachInterrupt(pin, ISR_BackButtonPress, 2);
      break;
    case up_btn:
      attachInterrupt(pin, ISR_UpButtonPress, 2);
      break;
    case down_btn:
      attachInterrupt(pin, ISR_DownButtonPress, 2);
      break;
    default:
      break;
  }
}
