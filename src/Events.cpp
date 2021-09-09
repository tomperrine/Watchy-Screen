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
  LOGI("start %d", id);
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
      default:
        break;
    }
  }
}

void send(ID eventID) {
  LOGD();
  esp_event_post_to(loop, BASE, eventID, nullptr, 0, 0);
  LOGD();
}

/**
 * @brief eventTask sits reading events from the queue and dispatching them as
 *        esp events
 *
 * @param p unused
 */
void producer(void *p) {
  uint32_t status;
  LOGD();
  while (true) {
    xTaskNotifyWait(0x00, ULONG_MAX, &status, portMAX_DELAY);
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
  LOGI();
  esp_event_loop_args_t args = {
      .queue_size = 8,
      .task_name = "Event dispatcher",
      .task_priority = ESP_TASKD_EVENT_PRIO,
      .task_stack_size = ESP_TASKD_EVENT_STACK,
      .task_core_id = tskNO_AFFINITY,
  };
  ESP_ERROR_CHECK(esp_event_loop_create(&args, &loop));
  ESP_ERROR_CHECK(esp_event_handler_register_with(loop, BASE, ESP_EVENT_ANY_ID,
                                                  handler, nullptr));

  buttonSetup(MENU_BTN_PIN, menu_btn);
  buttonSetup(BACK_BTN_PIN, back_btn);
  buttonSetup(UP_BTN_PIN, up_btn);
  buttonSetup(DOWN_BTN_PIN, down_btn);

  BaseType_t res = xTaskCreate(producer, "Event producer", 8192, nullptr,
                               ESP_TASK_MAIN_PRIO, &producerTask);
  configASSERT(producerTask);
  if (res != pdPASS) {
    LOGD("task create result %d", res);
  }
}
}  // namespace Watchy_Event