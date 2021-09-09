#include "Events.h"

#include <WiFi.h>
#include <esp_task.h>
#include <esp_task_wdt.h>

#include "Screen.h"
#include "Watchy.h"
#include "button_interrupt.h"

#define ESP_ERROR_CHECK(x)                          \
  do {                                              \
    esp_err_t __err_rc = (x);                       \
    if (__err_rc != ESP_OK) {                       \
      LOGE("%s %s", esp_err_to_name(__err_rc), #x); \
    }                                               \
  } while (0)

esp_event_loop_handle_t loop_handle;

ESP_EVENT_DEFINE_BASE(WATCHY_EVENT_BASE);

static void watchy_handler(void *handler_args, esp_event_base_t base,
                           int32_t id, void *event_data) {
  LOGI("watchy_handler start %d", id);
  if (Watchy::screen == nullptr) {
    LOGI("screen is null");
  } else {
    switch ((WATCHY_EVENT_ID)id) {
      case WATCHY_EVENT_MENU_BTN_DOWN:
        Watchy::screen->menu();
        break;
      case WATCHY_EVENT_BACK_BTN_DOWN:
        Watchy::screen->back();
        break;
      case WATCHY_EVENT_UP_BTN_DOWN:
        Watchy::screen->up();
        break;
      case WATCHY_EVENT_DOWN_BTN_DOWN:
        Watchy::screen->down();
        break;
      default:
        break;
    }
  }
  LOGI("watchy_handler end %d", id);
}

void send_event(WATCHY_EVENT_ID eventID) {
  LOGD();
  esp_event_post_to(loop_handle, WATCHY_EVENT_BASE, eventID, nullptr, 0, 0);
  LOGD();
}

/**
 * @brief eventTask sits reading events from the queue and dispatching them as
 *        esp events
 *
 * @param p unused
 */
void eventTask(void *p) {
  LOGD();
  while (true) {
    switch (buttonGet()) {
      case menu_btn:
        send_event(WATCHY_EVENT_MENU_BTN_DOWN);
        break;
      case back_btn:
        send_event(WATCHY_EVENT_BACK_BTN_DOWN);
        break;
      case up_btn:
        send_event(WATCHY_EVENT_UP_BTN_DOWN);
        break;
      case down_btn:
        send_event(WATCHY_EVENT_DOWN_BTN_DOWN);
        break;
      default:
        break;
    }
  }
}

esp_err_t event_handler(void *ctx, system_event_t *event) {
  LOGI("%d", (event ? event->event_id : -1));
  return ESP_OK;
}

void start_event_handler(void) {
  LOGI();
  esp_event_loop_args_t args = {
      .queue_size = 8,
      .task_name = "Event dispatcher",
      .task_priority = ESP_TASKD_EVENT_PRIO,
      .task_stack_size = ESP_TASKD_EVENT_STACK,
      .task_core_id = tskNO_AFFINITY,
  };
  ESP_ERROR_CHECK(esp_event_loop_create(&args, &loop_handle));
  ESP_ERROR_CHECK(esp_event_handler_register_with(
      loop_handle, WATCHY_EVENT_BASE, ESP_EVENT_ANY_ID, watchy_handler,
      nullptr));

  buttonSetup(MENU_BTN_PIN, menu_btn);
  buttonSetup(BACK_BTN_PIN, back_btn);
  buttonSetup(UP_BTN_PIN, up_btn);
  buttonSetup(DOWN_BTN_PIN, down_btn);

  TaskHandle_t task;
  BaseType_t res =
      xTaskCreate(eventTask, "Event producer", 8192, nullptr, ESP_TASK_MAIN_PRIO, &task);
  configASSERT(task);
  if (res != pdPASS) {
    LOGD("task create result %d", res);
  }
}