#include "Events.h"

#include <ctime>

#include <WiFi.h>
#include <esp_task.h>
#include <esp_task_wdt.h>

#include "Screen.h"
#include "Watchy.h"
#include "button_interrupt.h"

namespace Watchy_Event {

ESP_EVENT_DEFINE_BASE(BASE);

RTC_DATA_ATTR uint32_t updateInterval = 0;

void setUpdateInterval(uint32_t ms) {
  updateInterval = ms;  
}

void enableUpdateTimer() {
  if (updateInterval == 0) {
    return;
  }
#ifdef ESP_RTC
  log_i("wake on esp timer %lld", uint64_t(ms) * 1000ULL);
  esp_sleep_enable_timer_wakeup(uint64_t(ms) * 1000ULL);
  updateInterval = ms;
#else
  if (updateInterval % 1000 != 0 || updateInterval >= SECS_PER_DAY * 1000) {
    log_i("esp_sleep_enable_timer_wakeup(%llu)", uint64_t(updateInterval) * 1000ULL);
    esp_sleep_enable_timer_wakeup(uint64_t(updateInterval) * 1000ULL);
    return;
  }
  time_t t = ((time(nullptr)/(updateInterval/1000))+1)*(updateInterval/1000);
  uint8_t err;
  tmElements_t tm;
  // uint32_t oldFlags = Wire.setDebugFlags(16<<24,0);
  err = Watchy::RTC.read(tm);
  if (err) {
    log_e("error reading DTC %d", err);
  }
  err = Watchy::RTC.read(tm); // DEBUG
  if (err) {
    log_e("error reading DTC %d", err);
  }
  // Wire.setDebugFlags(oldFlags, ~oldFlags);
  log_i("     %d %02d:%02d:%02d", tm.Wday, tm.Hour, tm.Minute, tm.Second);
  log_i("%4d %d %02d:%02d:%02d", t-time(nullptr), dayOfWeek(t), numberOfHours(t), numberOfMinutes(t), numberOfSeconds(t));
  assert(tm.Wday <= 7);
  assert(tm.Hour < 24);
  assert(tm.Minute < 60);
  assert(tm.Second < 60);
  Watchy::RTC.setAlarm(ALM1_MATCH_DAY, numberOfSeconds(t), numberOfMinutes(t),
                       numberOfHours(t), dayOfWeek(t));
  Watchy::RTC.alarmInterrupt(ALARM_1, true);   // set/reset alarm interrupt
  log_i("esp_sleep_enable_ext0_wakeup(%d, %d)", RTC_PIN, 0);
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0);      // enable deep sleep wake on RTC interrupt
  log_i("stack high water %d", uxTaskGetStackHighWaterMark(nullptr));
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
    case LOCATION_UPDATE: return "LOCATION_UPDATE";
    case TIME_SYNC: return "TIME_SYNC";
    case MAX: return "MAX";
    default: return "unknown";
  }
}

// we're not going to actually run as a background task, this is just to keep
// from deep sleeping while we're running.
void handler(void *handler_args, esp_event_base_t base, int32_t id,
             void *event_data);

BackgroundTask handlerTask("handler", nullptr);

void handler(void *handler_args, esp_event_base_t base, int32_t id,
             void *event_data) {
  log_i("%6d handle event %s 0x%08x", millis(), IDtoString(static_cast<ID>(id)),
        event_data);
  if (Watchy::screen != nullptr) {
    handlerTask.add();
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
      case LOCATION_UPDATE:
        Watchy_GetLocation::currentLocation = reinterpret_cast<Event*>(event_data)->data.loc;
        break;
      case TIME_SYNC: 
      {
        log_i("time sync: %lu %lu", reinterpret_cast<Event *>(event_data)->data.tv.tv_sec,
              reinterpret_cast<Event *>(event_data)->data.tv.tv_usec);
        timeval *tv = &(reinterpret_cast<Event *>(event_data)->data.tv);
        // consider using tv.tv_usec as well
        Watchy::RTC.set(tv->tv_sec);  // set RTC
        setTime(tv->tv_sec);          // set system time
        settimeofday(tv, nullptr);    // set posix
        log_i("now %ld", now());
      }; 
      break;
      default:
        log_e("unknown event id: %d", id);
        break;
    }
    handlerTask.remove();
  }
}

void Event::send() {
  log_i("send event %s 0x%08x", IDtoString(id), this);
  esp_event_post_to(eventLoop, BASE, id, reinterpret_cast<void*>(this), sizeof(Event), 0);
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
      if (!BackgroundTask::running()) {
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
        Watchy_Event::Event{ .id = Watchy_Event::MENU_BTN_DOWN }.send();
        break;
      case back_btn:
        Watchy_Event::Event{ .id = Watchy_Event::BACK_BTN_DOWN }.send();
        break;
      case up_btn:
        Watchy_Event::Event{ .id = Watchy_Event::UP_BTN_DOWN }.send();
        break;
      case down_btn:
        Watchy_Event::Event{ .id = Watchy_Event::DOWN_BTN_DOWN }.send();
        break;
      default:
        break;
    }
  }
}

std::vector<const BackgroundTask*> BackgroundTask::tasks;
xSemaphoreHandle taskVectorLock = xSemaphoreCreateMutex();

void BackgroundTask::add() const {
  xSemaphoreTake(taskVectorLock, portMAX_DELAY);
  log_d("%6d %s %08x", millis(), name, reinterpret_cast<uint32_t>(this));
  tasks.push_back(this);
  xSemaphoreGive(taskVectorLock);
}

void BackgroundTask::remove() const {
  xSemaphoreTake(taskVectorLock, portMAX_DELAY);
  log_d("%6d %s %08x", millis(), name, reinterpret_cast<uint32_t>(this));
  for (auto it = tasks.begin(); it != tasks.end(); it++) {
    if (*it == this) {
      it = tasks.erase(it);
      break;
    }
  }
  xSemaphoreGive(taskVectorLock);
}

bool BackgroundTask::running() {
  xSemaphoreTake(taskVectorLock, portMAX_DELAY);
  bool r = tasks.size() > 0;
  xSemaphoreGive(taskVectorLock);
  return r;
}

void BackgroundTask::begin() {
  add();
  BaseType_t res = xTaskCreatePinnedToCore(
      [](void *p) {
        BackgroundTask *b = static_cast<BackgroundTask*>(p);
        b->taskFunction(nullptr);
        b->kill();
      },
      name, 4096, reinterpret_cast<void *>(this), tskIDLE_PRIORITY, &task, 1);
  configASSERT(task);
  if (res != pdPASS) {
    log_d("create background task result %d", res);
  }
}
void BackgroundTask::kill() {
  if (task != nullptr) {
    TaskHandle_t t = task;
    log_i("stack high water %d", uxTaskGetStackHighWaterMark(t));
    task = nullptr;
    remove();
    vTaskDelete(t); // doesn't return, because it's deleting us...
  }
}

BackgroundTask::~BackgroundTask() {
  kill();
}

TaskHandle_t producerTask;

// #define ESP_ERROR_CHECK(x) do { esp_err_t __err_rc = (x); if (__err_rc != ESP_OK) { log_e("err: %s", esp_err_to_name(__err_rc)); } } while(0);

esp_event_loop_handle_t eventLoop;

void start(void) {
  Watchy::initTime(); // so we can use now() in rate limits
  esp_event_loop_args_t eventLoopArgs = {
    .queue_size = 8,
    .task_name = "Event",
    .task_priority = ESP_TASKD_EVENT_PRIO,
    .task_stack_size = CONFIG_MAIN_TASK_STACK_SIZE,
    .task_core_id = 1, // "APP" core
  };
  ESP_ERROR_CHECK(esp_event_loop_create(&eventLoopArgs, &eventLoop));
  ESP_ERROR_CHECK(
      esp_event_handler_register_with(eventLoop, BASE, ESP_EVENT_ANY_ID, handler, nullptr));

  buttonSetup(MENU_BTN_PIN, menu_btn);
  buttonSetup(BACK_BTN_PIN, back_btn);
  buttonSetup(UP_BTN_PIN, up_btn);
  buttonSetup(DOWN_BTN_PIN, down_btn);
  // register for RTC gpio to send screen update events during long running
  // tasks. Figure out how to do this for ESP RTC wakeup timer too.

  BaseType_t res = xTaskCreate(producer, "Event producer", 3072, nullptr,
                               ESP_TASKD_EVENT_PRIO, &producerTask);
  configASSERT(producerTask);
  if (res != pdPASS) {
    log_e("create producer task failed: %d", res);
  }
}
}  // namespace Watchy_Event