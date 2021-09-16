#include "Events.h"

#include <WiFi.h>
#include <esp_task.h>
#include <esp_task_wdt.h>

#include <vector>

#include "Screen.h"
#include "Watchy.h"
#include "button_interrupt.h"

namespace Watchy_Event {

esp_event_loop_handle_t loop;

ESP_EVENT_DEFINE_BASE(BASE);

std::vector<TaskFunction_t> tasks;

void addBGTask(TaskFunction_t t) {
  log_d("%08x", reinterpret_cast<uint32_t>(t));
  tasks.push_back(t);
}

void removeBGTask(TaskFunction_t t) {
  log_d("%08x", reinterpret_cast<uint32_t>(t));
  for (auto it = tasks.begin(); it != tasks.end(); it++) {
    if (*it == t) {
      it = tasks.erase(it);
      break;
    }
  }
}

void setUpdateInterval(uint32_t ms) {
#ifdef ESP_RTC
  esp_sleep_enable_timer_wakeup(ms * 1000);
#else
  bool alarm1 = false;
  bool alarm2 = false;
  if (ms == 60000) {
    Watchy::RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);
    alarm2 = true;
  } else if (ms == 1000) {
    Watchy::RTC.setAlarm(ALM1_EVERY_SECOND, 0, 0, 0, 0);
    alarm1 = true;
  } else if (ms % 1000 != 0 || ms >= SECS_PER_DAY * 1000) {
    esp_sleep_enable_timer_wakeup(ms * 1000);
  } else if (ms > 0) {
    ms /= 1000;
    byte secs = ms % 60;
    ms /= 60;
    byte mins = ms % 60;
    ms /= 60;
    byte hours = ms % 24;
    byte days = ms / 24;
    ALARM_TYPES_t alarmType;
    if (secs != 0) {
      // has to be alarm 1 if it has secs
      if (days != 0) {
        alarmType = ALM1_MATCH_DAY;
      } else if (hours != 0) {
        alarmType = ALM1_MATCH_HOURS;
      } else if (mins != 0) {
        alarmType = ALM1_MATCH_MINUTES;
      } else {
        alarmType = ALM1_MATCH_SECONDS;
      }
      alarm1 = true;
    } else {
      // no secs, can be alarm 2
      if (days != 0) {
        alarmType = ALM2_MATCH_DAY;
      } else if (hours != 0) {
        alarmType = ALM2_MATCH_HOURS;
      } else {
        alarmType = ALM2_MATCH_MINUTES;
      }
      alarm2 = true;
    }
    Watchy::RTC.setAlarm(alarmType, secs, mins, hours, days);
  }
  Watchy::RTC.alarmInterrupt(ALARM_1, alarm1);  // enable alarm interrupt
  Watchy::RTC.alarmInterrupt(ALARM_2, alarm2);  // enable alarm interrupt
#endif
}

void handler(void *handler_args, esp_event_base_t base, int32_t id,
             void *event_data) {
  log_i("handle event %d", id);
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

void send(ID eventID, void *eventData) {
  log_i("send event %d %08x", eventID, (uint32_t)eventData);
  esp_event_post(BASE, eventID, eventData, 0, 0);
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
  const unsigned long DEBOUNCE_INTERVAL = 150000;  // 150ms
  uint32_t status;
  while (true) {
    auto v = xTaskNotifyWait(0x00, ULONG_MAX, &status, pdMS_TO_TICKS(5000));
    if (v != pdPASS) {
      if (tasks.size() == 0) {
        // no tasks outstanding
        Watchy::deepSleep();  // after 5 seconds with no events
      };
      log_i("%d tasks waiting", tasks.size());
      continue;
    }
    if (lastStatus == status && micros() <= lastMicros + DEBOUNCE_INTERVAL) {
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

TaskHandle_t beginBackgroundTask(TaskFunction_t f) {
  TaskHandle_t t;
  addBGTask(f);
  char name[] = "BG Task BEADFACE";
  sprintf(&name[8], "%08x", reinterpret_cast<uint32_t>(f));
  BaseType_t res = xTaskCreatePinnedToCore(
      [](void *p) {
        reinterpret_cast<TaskFunction_t>(p)(nullptr);
        removeBGTask(reinterpret_cast<TaskFunction_t>(p));
        vTaskDelete(nullptr);
      },
      name, 4096, reinterpret_cast<void *>(f), tskIDLE_PRIORITY, &t, 1);
  configASSERT(t);
  if (res != pdPASS) {
    log_d("create background task result %d", res);
  }
  return t;
}

TaskHandle_t producerTask;

void start(void) {
  ESP_ERROR_CHECK(
      esp_event_loop_create_default());  // does this interfere with elsewhere?
  ESP_ERROR_CHECK(
      esp_event_handler_register(BASE, ESP_EVENT_ANY_ID, handler, nullptr));

  buttonSetup(MENU_BTN_PIN, menu_btn);
  buttonSetup(BACK_BTN_PIN, back_btn);
  buttonSetup(UP_BTN_PIN, up_btn);
  buttonSetup(DOWN_BTN_PIN, down_btn);

  BaseType_t res = xTaskCreate(producer, "Event producer", 2048, nullptr,
                               ESP_TASKD_EVENT_PRIO, &producerTask);
  configASSERT(producerTask);
  if (res != pdPASS) {
    log_d("task create result %d", res);
  }
}
}  // namespace Watchy_Event