#include "button_interrupt.h"

#include <freertos/queue.h>

#include "Events.h"

#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 150
#endif

void IRAM_ATTR ISR_buttonPress(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  // xQueueSendFromISR(xQueue, (ButtonIndex *)&arg, &xHigherPriorityTaskWoken);
  xTaskNotifyFromISR(Watchy_Event::producerTask, (uint32_t)arg,
                     eSetValueWithoutOverwrite, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void buttonSetup(int pin, ButtonIndex index) {
  pinMode(pin, INPUT_PULLUP);
  attachInterruptArg(pin, ISR_buttonPress, (void *)index, 2);
}
