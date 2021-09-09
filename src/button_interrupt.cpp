#include "button_interrupt.h"

#include <freertos/queue.h>

#ifndef BUTTON_DEBOUNCE_MS
#define BUTTON_DEBOUNCE_MS 150
#endif

QueueHandle_t xQueue = xQueueCreate(1, sizeof(ButtonIndex));

void IRAM_ATTR ISR_buttonPress(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(xQueue, (ButtonIndex *)&arg, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

void buttonSetup(int pin, ButtonIndex index) {
  pinMode(pin, INPUT_PULLUP);
  attachInterruptArg(pin, ISR_buttonPress, (void *)index, 2);
}

int8_t buttonGet() {
  ButtonIndex b;
  do { } while (!xQueueReceive(xQueue, &b, portMAX_DELAY));
  return b;
}