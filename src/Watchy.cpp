#include "Watchy.h"

#include <vector>

#include "Events.h"
#include "GetLocation.h"  // bad dependency
#include "Screen.h"
#include "WatchyErrors.h"

namespace Watchy {

Error err;

void _rtcConfig();
void _bmaConfig();
uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                       uint16_t len);
uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                        uint16_t len);

DS3232RTC RTC(false);
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
    GxEPD2_154_D67(CS, DC, RESET, BUSY));
RTC_DATA_ATTR Screen *screen = nullptr;

RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR bool BLE_CONFIGURED;

String getValue(String data, char separator, int index) {
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

void handleButtonPress() {
  uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
  switch (wakeupBit & BTN_PIN_MASK) {
    case MENU_BTN_MASK:
      Watchy_Event::send(Watchy_Event::MENU_BTN_DOWN);
      break;
    case BACK_BTN_MASK:
      Watchy_Event::send(Watchy_Event::BACK_BTN_DOWN);
      break;
    case UP_BTN_MASK:
      Watchy_Event::send(Watchy_Event::UP_BTN_DOWN);
      break;
    case DOWN_BTN_MASK:
      Watchy_Event::send(Watchy_Event::DOWN_BTN_DOWN);
      break;
    default:
      break;
  }
}

tmElements_t currentTime;  // should probably be in SyncTime

// doesn't persist over deep sleep. don't care.
std::vector<OnWakeCallback> owcVec;

void AddOnWakeCallback(const OnWakeCallback owc) {
  owcVec.push_back(owc);
}

const char * wakeupReasonToString(esp_sleep_wakeup_cause_t wakeup_reason) {
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_UNDEFINED: return("UNDEFINED");
    case ESP_SLEEP_WAKEUP_ALL: return("ALL");
    case ESP_SLEEP_WAKEUP_EXT0: return("EXT0");
    case ESP_SLEEP_WAKEUP_EXT1: return("EXT1");
    case ESP_SLEEP_WAKEUP_TIMER: return("TIMER");
    case ESP_SLEEP_WAKEUP_TOUCHPAD: return("TOUCHPAD");
    case ESP_SLEEP_WAKEUP_ULP: return("ULP");
    case ESP_SLEEP_WAKEUP_GPIO: return("GPIO");
    case ESP_SLEEP_WAKEUP_UART: return("UART");
    default: return("unknown");
  }
}

uint64_t start;

void init() {
  start = micros();
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();  // get wake up reason
  Wire.begin(SDA, SCL);                          // init i2c
  log_i("reason %s", wakeupReasonToString(wakeup_reason));

  // sync ESP32 clocks to RTC
  if (RTC.read(currentTime) == 0) {
    setenv("TZ", Watchy_GetLocation::currentLocation.timezone, 1);
    tzset();
    time_t t = makeTime(currentTime);
    setTime(t);
    timeval tv = {t, 0};
    settimeofday(&tv, nullptr);
  }

  for (auto &&owc : owcVec) {
    owc(wakeup_reason);
  }

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_TIMER:  // ESP Internal RTC
#ifdef ESP_RTC
      tmElements_t currentTime;
      RTC.read(currentTime);
      currentTime.Minute++;
      tmElements_t tm;
      tm.Month = currentTime.Month;
      tm.Day = currentTime.Day;
      tm.Year = currentTime.Year;
      tm.Hour = currentTime.Hour;
      tm.Minute = currentTime.Minute;
      tm.Second = 0;
      time_t t = makeTime(tm);
      RTC.set(t);
#endif
      Watchy_Event::send(Watchy_Event::UPDATE_SCREEN);
      break;
    case ESP_SLEEP_WAKEUP_EXT0:  // RTC Alarm
      RTC.alarm(ALARM_1);        // resets the alarm flag in the RTC
      RTC.alarm(ALARM_2);        // resets the alarm flag in the RTC
      Watchy_Event::send(Watchy_Event::UPDATE_SCREEN);
      break;
    case ESP_SLEEP_WAKEUP_EXT1:  // button Press
      handleButtonPress();
      break;
    default:  // reset
      _bmaConfig();
      showWatchFace(false);  // full update on reset
      break;
  }
  // deepSleep();
}

void deepSleep() {
  uint64_t elapsed = micros()-start;
  display.hibernate();
  _rtcConfig();
  esp_sleep_enable_ext1_wakeup(
      BTN_PIN_MASK,
      ESP_EXT1_WAKEUP_ANY_HIGH);  // enable deep sleep wake on button press
  log_i("*** sleeping %llu.%03llu ***\n", elapsed/1000, elapsed%1000);
  esp_deep_sleep_start();
}

void _rtcConfig() {
#ifndef ESP_RTC
  // https://github.com/JChristensen/DS3232RTC
  RTC.squareWave(SQWAVE_NONE);  // disable square wave output
#endif
}

void showWatchFace(bool partialRefresh, Screen *s) {
  display.init(
      0, false);  //_initial_refresh to false to prevent full update on init
  display.setFullWindow();
  display.setTextColor((s->bgColor == GxEPD_WHITE ? GxEPD_BLACK : GxEPD_WHITE));
  display.setCursor(0, 0);
  s->show();
  display.display(partialRefresh);  // partial refresh
}

const Screen *getScreen() {
  return screen;
}

// setScreen is used to set a new screen on the display
void setScreen(Screen *s) {
  if (s == nullptr) {
    return;
  }
  screen = s;
  showWatchFace(true);
}

uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                       uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)address, (uint8_t)len);
  uint8_t i = 0;
  while (Wire.available()) {
    data[i++] = Wire.read();
  }
  return 0;
}

uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                        uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data, len);
  return (0 != Wire.endTransmission());
}

void _bmaConfig() {
  if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
    // fail to init BMA
    return;
  }

  // Accel parameter structure
  Acfg cfg;
  /*!
      Output data rate in Hz, Optional parameters:
          - BMA4_OUTPUT_DATA_RATE_0_78HZ
          - BMA4_OUTPUT_DATA_RATE_1_56HZ
          - BMA4_OUTPUT_DATA_RATE_3_12HZ
          - BMA4_OUTPUT_DATA_RATE_6_25HZ
          - BMA4_OUTPUT_DATA_RATE_12_5HZ
          - BMA4_OUTPUT_DATA_RATE_25HZ
          - BMA4_OUTPUT_DATA_RATE_50HZ
          - BMA4_OUTPUT_DATA_RATE_100HZ
          - BMA4_OUTPUT_DATA_RATE_200HZ
          - BMA4_OUTPUT_DATA_RATE_400HZ
          - BMA4_OUTPUT_DATA_RATE_800HZ
          - BMA4_OUTPUT_DATA_RATE_1600HZ
  */
  cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
  /*!
      G-range, Optional parameters:
          - BMA4_ACCEL_RANGE_2G
          - BMA4_ACCEL_RANGE_4G
          - BMA4_ACCEL_RANGE_8G
          - BMA4_ACCEL_RANGE_16G
  */
  cfg.range = BMA4_ACCEL_RANGE_2G;
  /*!
      Bandwidth parameter, determines filter configuration, Optional parameters:
          - BMA4_ACCEL_OSR4_AVG1
          - BMA4_ACCEL_OSR2_AVG2
          - BMA4_ACCEL_NORMAL_AVG4
          - BMA4_ACCEL_CIC_AVG8
          - BMA4_ACCEL_RES_AVG16
          - BMA4_ACCEL_RES_AVG32
          - BMA4_ACCEL_RES_AVG64
          - BMA4_ACCEL_RES_AVG128
  */
  cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

  /*! Filter performance mode , Optional parameters:
      - BMA4_CIC_AVG_MODE
      - BMA4_CONTINUOUS_MODE
  */
  cfg.perf_mode = BMA4_CONTINUOUS_MODE;

  // Configure the BMA423 accelerometer
  sensor.setAccelConfig(cfg);

  // Enable BMA423 accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  sensor.enableAccel();

  struct bma4_int_pin_config config;
  config.edge_ctrl = BMA4_LEVEL_TRIGGER;
  config.lvl = BMA4_ACTIVE_HIGH;
  config.od = BMA4_PUSH_PULL;
  config.output_en = BMA4_OUTPUT_ENABLE;
  config.input_en = BMA4_INPUT_DISABLE;
  // The correct trigger interrupt needs to be configured as needed
  sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

  struct bma423_axes_remap remap_data;
  remap_data.x_axis = 1;
  remap_data.x_axis_sign = 0xFF;
  remap_data.y_axis = 0;
  remap_data.y_axis_sign = 0xFF;
  remap_data.z_axis = 2;
  remap_data.z_axis_sign = 0xFF;
  // Need to raise the wrist function, need to set the correct axis
  sensor.setRemapAxes(&remap_data);

  // Enable BMA423 isStepCounter feature
  sensor.enableFeature(BMA423_STEP_CNTR, true);
  // Enable BMA423 isTilt feature
  sensor.enableFeature(BMA423_TILT, true);
  // Enable BMA423 isDoubleClick feature
  sensor.enableFeature(BMA423_WAKEUP, true);

  // Reset steps
  sensor.resetStepCounter();

  // Turn on feature interrupt
  sensor.enableStepCountInterrupt();
  sensor.enableTiltInterrupt();
  // It corresponds to isDoubleClick interrupt
  sensor.enableWakeupInterrupt();
}

RTC_DATA_ATTR bool wifiReset;
auto wifiMutex = xSemaphoreCreateMutex();

bool connectWiFi() {
  // in theory this is re-entrant, but in practice if you call WiFi.begin()
  // while it's still trying to connect, it will return an error. Better
  // to serialize WiFi.begin()
  xSemaphoreTake(wifiMutex, portMAX_DELAY);
  if (!wifiReset) {
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_config);
    wifiReset = true;
  }
#if !defined(WIFI_SSID) || !defined(WIFI_PASSWORD)
  if (WL_CONNECT_FAILED == WiFi.begin()) {
    // WiFi not setup, you can also use hard coded credentials with
    // WiFi.begin(SSID,PASS); by defining WIFI_SSID and WIFI_PASSWORD
#else
  if (WL_CONNECT_FAILED == WiFi.begin() &&
      WL_CONNECT_FAILED == WiFi.begin(WIFI_SSID, WIFI_PASSWORD)) {
    // WiFi not setup
#endif
    WIFI_CONFIGURED = false;
  } else {
    if (WL_CONNECTED ==
        WiFi.waitForConnectResult()) {  // attempt to connect for 10s
      WIFI_CONFIGURED = true;
    } else {  // connection failed, time out
      WIFI_CONFIGURED = false;
      // turn off radios
      WiFi.mode(WIFI_OFF);
      btStop();
    }
  }
  xSemaphoreGive(wifiMutex);
  return WIFI_CONFIGURED;
}

unsigned int wifiConnectionCount = 0;

bool getWiFi() {
  if (wifiConnectionCount > 0 || connectWiFi()) {
    if (wifiConnectionCount == 0) {
      log_d("wifi connected");
    }
    wifiConnectionCount++;
    return true;
  }
  return false;
}

void releaseWiFi() {
  wifiConnectionCount--;
  if (wifiConnectionCount > 0) {
    return;
  }
  log_d("wifi disconnected");
  btStop();
  WiFi.mode(WIFI_OFF);
}
}  // namespace Watchy