#include "WatchyRTC.h"
#include "config.h"

#define RTC_DS_ADDR 0x68
#define RTC_PCF_ADDR 0x51
#define YEAR_OFFSET_DS 1970
#define YEAR_OFFSET_PCF 2000

RTC_DATA_ATTR RTC_REFRESH_t WatchyRTC::_refresh = RTC_REFRESH_NONE;

WatchyRTC::WatchyRTC() 
  : rtc_ds(false) {}

void WatchyRTC::init() {
  byte error;
  Wire.beginTransmission(RTC_DS_ADDR);
  error = Wire.endTransmission();
  if (error == 0) {
    rtcType = DS3231;
  } else {
    Wire.beginTransmission(RTC_PCF_ADDR);
    error = Wire.endTransmission();
    if (error == 0) {
      rtcType = PCF8563;
    } else {
      log_e("RTC Error: %d", error);
    }
  }
}

void WatchyRTC::config(String datetime) {
  if (rtcType == DS3231) {
    _DSConfig(datetime);
  } else {
    _PCFConfig(datetime);
  }
  setRefresh(RTC_REFRESH_MIN);
}

void WatchyRTC::clearAlarm() {
  if (rtcType == DS3231) {
    rtc_ds.alarm(ALARM_2);
  } else {
    rtc_pcf.clearAlarm();  // resets the alarm flag in the RTC
  }
}

void WatchyRTC::read(tmElements_t &tm) {
  if (rtcType == DS3231) {
    rtc_ds.read(tm);
    tm.Year = tm.Year - 30;  // reset to offset from 2000
  } else {
    tm.Month = rtc_pcf.getMonth();
    if (tm.Month == 0) {  // PCF8563 POR sets month = 0 for some reason
      tm.Month = 1;
      tm.Year = 21;
    } else {
      tm.Year = rtc_pcf.getYear();
    }
    tm.Day = rtc_pcf.getDay();
    tm.Wday = rtc_pcf.getWeekday() + 1;
    tm.Hour = rtc_pcf.getHour();
    tm.Minute = rtc_pcf.getMinute();
    tm.Second = rtc_pcf.getSecond();
  }
}

void WatchyRTC::set(tmElements_t tm) {
  if (rtcType == DS3231) {
    tm.Year = tm.Year + 2000 - YEAR_OFFSET_DS;
    time_t t = makeTime(tm);
    rtc_ds.set(t);
  } else {
    rtc_pcf.setDate(tm.Day,
                    _getDayOfWeek(tm.Day, tm.Month, tm.Year + YEAR_OFFSET_PCF),
                    tm.Month, 0, tm.Year);
    rtc_pcf.setTime(tm.Hour, tm.Minute, tm.Second);
    clearAlarm();
  }
}

void WatchyRTC::setAlarm(uint8_t minutes, uint8_t hours, uint8_t dayOfWeek) {
  log_d("setAlarm(%d,%d,%d", minutes, hours, dayOfWeek);
  if (rtcType == DS3231) {
    rtc_ds.setAlarm(ALM1_MATCH_MINUTES, 0, minutes, hours, dayOfWeek);
    rtc_ds.alarmInterrupt(ALARM_1, true);
  } else {
    rtc_pcf.setAlarm(minutes, hours, 99, dayOfWeek);
  }
}

void WatchyRTC::setRefresh(RTC_REFRESH_t r) {
  log_d("refresh(%d)", r);
  if ((r == RTC_REFRESH_SEC) || (r == RTC_REFRESH_MIN)) {
    // enable wakeup interrupt
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0);
  } else if ((refresh() == RTC_REFRESH_SEC) || (refresh() == RTC_REFRESH_MIN)) {
    // disable wakeup interrupt
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
  }
  if (refresh() == r) {
    return;
  }
  if (rtcType == DS3231) {
    switch (r) {
      case RTC_REFRESH_SEC:
        rtc_ds.setAlarm(ALM1_EVERY_SECOND, 0, 0, 0, 0);
        rtc_ds.alarmInterrupt(1, true);
        if (refresh() == RTC_REFRESH_MIN) {
          rtc_ds.alarmInterrupt(2, false);
        }
        break;
      case RTC_REFRESH_MIN:
        rtc_ds.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);
        rtc_ds.alarmInterrupt(2, true);
        if (refresh() == RTC_REFRESH_SEC) {
          rtc_ds.alarmInterrupt(1, false);
        }
        break;
      default:  // either fast or none
        rtc_ds.alarmInterrupt(1, false);
        rtc_ds.alarmInterrupt(2, false);
        break;
    }
  } else {
    switch (r) {
      case RTC_REFRESH_SEC:
        rtc_pcf.setTimer(1, TMR_1Hz, true);
        break;
      case RTC_REFRESH_MIN:
        rtc_pcf.setTimer(1, TMR_1MIN, true);
        break;
      default:  // either fast or none
        rtc_pcf.clearTimer();
        break;
    }
  }
  WatchyRTC::_refresh = r;
}

uint8_t WatchyRTC::temperature() {
  if (rtcType == DS3231) {
    return rtc_ds.temperature();
  } else {
    return 255;  // error
  }
}

void WatchyRTC::_DSConfig(String datetime) {
  if (datetime != "") {
    tmElements_t tm;
    tm.Year =
        _getValue(datetime, ':', 0).toInt() -
        YEAR_OFFSET_DS;  // offset from 1970, since year is stored in uint8_t
    tm.Month = _getValue(datetime, ':', 1).toInt();
    tm.Day = _getValue(datetime, ':', 2).toInt();
    tm.Hour = _getValue(datetime, ':', 3).toInt();
    tm.Minute = _getValue(datetime, ':', 4).toInt();
    tm.Second = _getValue(datetime, ':', 5).toInt();
    time_t t = makeTime(tm);
    rtc_ds.set(t);
  }
  // https://github.com/JChristensen/DS3232RTC
  rtc_ds.squareWave(SQWAVE_NONE);  // disable square wave output
}

void WatchyRTC::_PCFConfig(String datetime) {
  if (datetime != "") {
    int Year = _getValue(datetime, ':', 0).toInt();
    int Month = _getValue(datetime, ':', 1).toInt();
    int Day = _getValue(datetime, ':', 2).toInt();
    int Hour = _getValue(datetime, ':', 3).toInt();
    int Minute = _getValue(datetime, ':', 4).toInt();
    int Second = _getValue(datetime, ':', 5).toInt();
    // day, weekday, month, century(1=1900, 0=2000), year(0-99)
    rtc_pcf.setDate(Day, _getDayOfWeek(Day, Month, Year), Month, 0,
                    Year - YEAR_OFFSET_PCF);  // offset from 2000
    // hr, min, sec
    rtc_pcf.setTime(Hour, Minute, Second);
  }
}

int WatchyRTC::_getDayOfWeek(int d, int m, int y) {
  // Sakamoto's method
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

String WatchyRTC::_getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}