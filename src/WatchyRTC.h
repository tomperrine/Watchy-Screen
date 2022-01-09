#ifndef WATCHY_RTC_H
#define WATCHY_RTC_H

#include <DS3232RTC.h>
#include <Rtc_Pcf8563.h>

enum RTC_t { DS3231, PCF8563 };

enum RTC_REFRESH_t {
  RTC_REFRESH_NONE,  // never refresh, wake on buttons only
  RTC_REFRESH_SEC,   // refresh every second
  RTC_REFRESH_MIN,   // refresh every minute
  RTC_REFRESH_FAST   // never refresh, never sleep
};

class WatchyRTC {
 public:
  union {
    DS3232RTC rtc_ds;
    Rtc_Pcf8563 rtc_pcf;
  };
  RTC_t rtcType;

 public:
  WatchyRTC();
  void init();
  void config(String datetime);
  void clearAlarm();
  void read(tmElements_t &tm);
  void set(tmElements_t tm);
  uint8_t temperature();
  void setAlarm(uint8_t minutes, uint8_t hours, uint8_t dayOfWeek);
  void setRefresh(RTC_REFRESH_t r);
  RTC_REFRESH_t refresh() { return _refresh; }

 private:
  void _DSConfig(String datetime);
  void _PCFConfig(String datetime);
  int _getDayOfWeek(int d, int m, int y);
  String _getValue(String data, char separator, int index);
  static RTC_REFRESH_t _refresh;
};

#endif