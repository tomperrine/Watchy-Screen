#include "Events.h"

#include <WiFi.h>
#include <esp_task.h>
#include <esp_task_wdt.h>

#include "Screen.h"
#include "Watchy.h"
#include "button_interrupt.h"

namespace Watchy_Event {

esp_event_loop_handle_t loop;

ESP_EVENT_DEFINE_BASE(BASE);

void handler(void *handler_args, esp_event_base_t base, int32_t id,
                    void *event_data) {
  LOGI("handle event %d", id);
  if (Watchy::screen != nullptr) {
    switch ((ID)id) {
      case MENU_BTN_DOWN:
        Watchy::screen->menu();
        break;
      case BACK_BTN_DOWN:
        Watchy::screen->back();
        break;
      case UP_BTN_DOWN:
        Watchy::screen->up();
        break;
      case DOWN_BTN_DOWN:
        Watchy::screen->down();
        break;
      case UPDATE_SCREEN:
        Watchy::showWatchFace(true);
        break;
      default:
        break;
    }
  }
}

void send(ID eventID) {
  LOGI("send event %d", eventID);
  esp_event_post(BASE, eventID, nullptr, 0, 0);
}

/**
 * @brief eventTask sits reading events from the queue and dispatching them as
 *        esp events
 *
 * @param p unused
 */
void producer(void *p) {
  static uint32_t lastStatus;
  static unsigned long lastMicros;
  const unsigned long DEBOUNCE_INTERVAL = 150000; // 150ms
  uint32_t status;
  while (true) {
    auto v = xTaskNotifyWait(0x00, ULONG_MAX, &status, pdMS_TO_TICKS(5000));
    if (v != pdPASS) {
        Watchy::deepSleep(); // after 5 seconds with no events
    }
    if (lastStatus == status && micros() <= lastMicros+DEBOUNCE_INTERVAL) {
      // bounce
      continue;
    }
    lastStatus = status;
    lastMicros = micros();
    switch ((ButtonIndex)status) {
      case menu_btn:
        send(MENU_BTN_DOWN);
        break;
      case back_btn:
        send(BACK_BTN_DOWN);
        break;
      case up_btn:
        send(UP_BTN_DOWN);
        break;
      case down_btn:
        send(DOWN_BTN_DOWN);
        break;
      default:
        break;
    }
  }
}

TaskHandle_t producerTask;

void start(void) {
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // does this interfere with elsewhere?
  ESP_ERROR_CHECK(esp_event_handler_register(BASE, ESP_EVENT_ANY_ID,
                                                  handler, nullptr));

  buttonSetup(MENU_BTN_PIN, menu_btn);
  buttonSetup(BACK_BTN_PIN, back_btn);
  buttonSetup(UP_BTN_PIN, up_btn);
  buttonSetup(DOWN_BTN_PIN, down_btn);

  BaseType_t res = xTaskCreate(producer, "Event producer", 2048, nullptr,
                               ESP_TASKD_EVENT_PRIO, &producerTask);
  configASSERT(producerTask);
  if (res != pdPASS) {
    LOGD("task create result %d", res);
  }
}
}  // namespace Watchy_Event