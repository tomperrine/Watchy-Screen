#include "SyncTime.h"

#include <sntp.h>

#include "GetLocation.h"
#include "Watchy.h"
#include "WatchyErrors.h"
#include "time.h"

namespace Watchy_SyncTime {

RTC_DATA_ATTR const char *ntpServer = NTP_SERVER;
RTC_DATA_ATTR time_t lastSyncTimeTS = 0;

// RTC does not know about TZ
// so DST has to be in app code
// so RTC must store UTC
// RTC computes leap year so you want year to be correct
// according to how the RTC represents it

void timeSyncCallback(struct timeval *tv) {
  // consider using tv.tv_usec as well
  Watchy::RTC.set(tv->tv_sec);  // set RTC
  setTime(tv->tv_sec);          // set system time
  settimeofday(tv, nullptr);    // set posix
  sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
  lastSyncTimeTS = tv->tv_sec;
  LOGD("lastSyncTimeTS %ld", lastSyncTimeTS);
}

void syncTime(const char* timezone) {
  if (!Watchy::connectWiFi()) {
    Watchy::err = Watchy::WIFI_FAILED;
    return;
  }
  if (sntp_get_sync_status() != SNTP_SYNC_STATUS_RESET) {
    // SNTP busy
    LOGI("%d", sntp_get_sync_status());
    Watchy::err = Watchy::NOT_READY;
    return;
  }
  sntp_set_time_sync_notification_cb(timeSyncCallback);
  configTzTime(timezone, ntpServer);
  uint32_t timeout = millis() + 5000;  // 5 sec timeout
  Watchy::err = Watchy::TIMEOUT;
  while (millis() < timeout) {
    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
      Watchy::err = Watchy::OK;
      break;
    }
    yield();  // or delay or get event
  }
  WiFi.mode(WIFI_OFF);
  btStop();
}
}  // namespace Watchy_SyncTime