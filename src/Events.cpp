#include "Events.h"

#include <WiFi.h>

#include "Screen.h"
#include "Watchy.h"

#define ESP_ERROR_CHECK(x) do {esp_err_t __err_rc = (x); if (__err_rc != ESP_OK) { LOGE("%s %s", esp_err_to_name(__err_rc), #x); }} while (0)

const char * eventIDtoString(system_event_id_t event) {
  switch (event) {
    case SYSTEM_EVENT_WIFI_READY:             return "WiFi ready";
    case SYSTEM_EVENT_SCAN_DONE:              return "finish scanning AP";
    case SYSTEM_EVENT_STA_START:              return "station start";
    case SYSTEM_EVENT_STA_STOP:               return "station stop";
    case SYSTEM_EVENT_STA_CONNECTED:          return "station connected to AP";
    case SYSTEM_EVENT_STA_DISCONNECTED:       return "station disconnected from AP";
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:    return "auth mode of AP connected by ESP32 station changed";
    case SYSTEM_EVENT_STA_GOT_IP:             return "station got IP from connected AP";
    case SYSTEM_EVENT_STA_LOST_IP:            return "station lost IP and the IP is reset to 0";
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:     return "station wps succeeds in enrollee mode";
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:      return "station wps fails in enrollee mode";
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:     return "station wps timeout in enrollee mode";
    case SYSTEM_EVENT_STA_WPS_ER_PIN:         return "station wps pin code in enrollee mode";
    case SYSTEM_EVENT_STA_WPS_ER_PBC_OVERLAP: return "station wps overlap in enrollee mode";
    case SYSTEM_EVENT_AP_START:               return "soft-AP start";
    case SYSTEM_EVENT_AP_STOP:                return "soft-AP stop";
    case SYSTEM_EVENT_AP_STACONNECTED:        return "a station connected to ESP32 soft-AP";
    case SYSTEM_EVENT_AP_STADISCONNECTED:     return "a station disconnected from ESP32 soft-AP";
    case SYSTEM_EVENT_AP_STAIPASSIGNED:       return "soft-AP assign an IP to a connected station";
    case SYSTEM_EVENT_AP_PROBEREQRECVED:      return "Receive probe request packet in soft-AP interface";
    case SYSTEM_EVENT_GOT_IP6:                return "station or ap or ethernet interface v6IP addr is preferred";
    case SYSTEM_EVENT_ETH_START:              return "ethernet start";
    case SYSTEM_EVENT_ETH_STOP:               return "ethernet stop";
    case SYSTEM_EVENT_ETH_CONNECTED:          return "ethernet phy link up";
    case SYSTEM_EVENT_ETH_DISCONNECTED:       return "ethernet phy link down";
    case SYSTEM_EVENT_ETH_GOT_IP:             return "ethernet got IP from connected AP";
    default: {
      static char buf[20];
      sprintf(buf, "unknown 0x%08x", event);
      return buf;
    }
  }
}

void wiFiProvEventCb(system_event_t *sys_event, wifi_prov_event_t *prov_event) {
  LOGI("%s", eventIDtoString(sys_event->event_id));
}

ESP_EVENT_DEFINE_BASE(WATCHY_EVENT_BASE);

static void watchy_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
  LOGI("watchy_handler start %d", id);
  switch ((WATCHY_EVENT_ID)id) {
    case WATCHY_EVENT_MENU_BTN_DOWN: Watchy::screen->menu(); break;
    case WATCHY_EVENT_BACK_BTN_DOWN: Watchy::screen->back(); break;
    case WATCHY_EVENT_UP_BTN_DOWN:   Watchy::screen->up();   break;
    case WATCHY_EVENT_DOWN_BTN_DOWN: Watchy::screen->down(); break;
    default: break;
  }
  LOGI("watchy_handler end %d", id);
}

void send_event(WATCHY_EVENT_ID eventID) {
  esp_event_post(WATCHY_EVENT_BASE, eventID, nullptr, 0, 0);
}

static system_event_cb_t old_cb;

esp_err_t system_event_cb(void *ctx, system_event_t *event) {
  LOGI("%08x, %08x", ctx, event);
  return old_cb(ctx, event);
}

void start_event_handler(void) {
  LOGI();

  // ignore returned event handler ids
  WiFi.onEvent(&wiFiProvEventCb);

  old_cb = esp_event_loop_set_cb(&system_event_cb, NULL);

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(
      WATCHY_EVENT_BASE, ESP_EVENT_ANY_ID, watchy_handler, nullptr));
}