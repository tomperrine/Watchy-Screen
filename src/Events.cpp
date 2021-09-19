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

RTC_DATA_ATTR uint32_t updateInterval = 0;

void setUpdateInterval(uint32_t ms) {
  if (ms == 0) {
    updateInterval = ms;
    return;
  }
#ifdef ESP_RTC
  log_i("wake on esp timer %lld", uint64_t(ms) * 1000ULL);
  esp_sleep_enable_timer_wakeup(uint64_t(ms) * 1000ULL);
  updateInterval = ms;
#else
  if (ms % 1000 != 0 || ms >= SECS_PER_DAY * 1000) {
    log_i("esp_sleep_enable_timer_wakeup(%llu)", uint64_t(ms) * 1000ULL);
    esp_sleep_enable_timer_wakeup(uint64_t(ms) * 1000ULL);
    updateInterval = ms;
    return;
  }
  updateInterval = ms;
  time_t t = ((time(nullptr)/(ms/1000))+1)*(ms/1000);
  Watchy::RTC.setAlarm(ALM1_MATCH_DAY, numberOfSeconds(t), numberOfMinutes(t),
                       numberOfHours(t), dayOfWeek(t));
  Watchy::RTC.alarmInterrupt(ALARM_1, true);   // set/reset alarm interrupt
  log_i("esp_sleep_enable_ext0_wakeup(%d, %d)", RTC_PIN, 0);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0);      // enable deep sleep wake on RTC interrupt
#endif
}

const char * IDtoString(ID id) {
  switch (id) {
    case NULL_EVENT: return "NULL_EVENT";
    case MENU_BTN_DOWN: return "MENU_BTN_DOWN";
    case BACK_BTN_DOWN: return "BACK_BTN_DOWN";
    case UP_BTN_DOWN: return "UP_BTN_DOWN";
    case DOWN_BTN_DOWN: return "DOWN_BTN_DOWN";
    case UPDATE_SCREEN: return "UPDATE_SCREEN";
    case MAX: return "MAX";
    default: return "unknown";
  }
}

void handler(void *handler_args, esp_event_base_t base, int32_t id,
             void *event_data) {
  log_i("handle event %s", IDtoString(static_cast<ID>(id)));
  if (Watchy::screen != nullptr) {
    addBGTask(reinterpret_cast<TaskFunction_t>(&handler));
    switch (static_cast<ID>(id)) {
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
    removeBGTask(reinterpret_cast<TaskFunction_t>(&handler));
  }
}

void send(ID eventID, void *eventData) {
  log_i("send event %s", IDtoString(eventID));
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
    auto v = xTaskNotifyWait(0x00, ULONG_MAX, &status, pdMS_TO_TICKS(50));
    if (v != pdPASS) {
      if (tasks.size() == 0) {
        // no tasks outstanding
        Watchy::deepSleep();
      };
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
    log_e("create producer task failed: %d", res);
  }
}
}  // namespace Watchy_Event