#include "Events.h"

#include <WiFi.h>

#include "Screen.h"
#include "Watchy.h"

#define ESP_ERROR_CHECK(x) do {esp_err_t __err_rc = (x); if (__err_rc != ESP_OK) { LOGE("%s %s", esp_err_to_name(__err_rc), #x); }} while (0)

// Handler which executes when any event occurs
static void any_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    LOGI("any_handler %s %d args %08x data %08x", base, id, (uint32_t)handler_args, (uint32_t)event_data);
}

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

QueueHandle_t xEventQueue;

void IRAM_ATTR btnISR(WATCHY_EVENT_ID event) {
  BaseType_t xHigherPriorityTaskWokenByPost = pdFALSE;
  xQueueSendFromISR(xEventQueue, (void *)&event,
                    &xHigherPriorityTaskWokenByPost);
  if (xHigherPriorityTaskWokenByPost) {
    portYIELD_FROM_ISR();
  }
}

void IRAM_ATTR btnMenuISR() { btnISR(WATCHY_EVENT_MENU_BTN_DOWN); }
void IRAM_ATTR btnBackISR() { btnISR(WATCHY_EVENT_BACK_BTN_DOWN); }
void IRAM_ATTR btnUpISR()   { btnISR(WATCHY_EVENT_UP_BTN_DOWN); }
void IRAM_ATTR btnDownISR() { btnISR(WATCHY_EVENT_DOWN_BTN_DOWN); }

void eventHandlerTask(void *pvParameters) {
  WATCHY_EVENT_ID eventID;
  if (xEventQueue == 0) { return; };  // error
  while (1) {
    eventID = WATCHY_EVENT_MAX;
    xQueueReceive(xEventQueue, &eventID, portMAX_DELAY);
    // Serial.print("eventHandlerTask "); Serial.println(eventID);
    Serial.print("eventID "); Serial.println(eventID);
    switch (eventID) {
      case WATCHY_EVENT_MENU_BTN_DOWN: Watchy::screen->menu(); break;
      case WATCHY_EVENT_BACK_BTN_DOWN: Watchy::screen->back(); break;
      case WATCHY_EVENT_UP_BTN_DOWN:   Watchy::screen->up();   break;
      case WATCHY_EVENT_DOWN_BTN_DOWN: Watchy::screen->down(); break;
      default: break;
    }
  }
}

void send_event(WATCHY_EVENT_ID eventID) {
  xQueueSend(xEventQueue, (void *)&eventID, 0);
}

void start_event_handler(void) {
  LOGI();

  xEventQueue = xQueueCreate(10, sizeof(WATCHY_EVENT_ID));

  xTaskCreate(eventHandlerTask,     /* Task function. */
              "WatchyEventHandler", /* String with name of task. */
              10000,                /* Stack size in words. */
              NULL,                 /* Parameter passed as input of the task */
              1,                    /* Priority of the task. */
              NULL);                /* Task handle. */

  send_event(WATCHY_EVENT_MAX); // DEBUG

  // enable interrupts for buttons
  attachInterrupt(MENU_BTN_PIN, &btnMenuISR, RISING);
  attachInterrupt(BACK_BTN_PIN, &btnBackISR, RISING);
  attachInterrupt(UP_BTN_PIN,   &btnUpISR,   RISING);
  attachInterrupt(DOWN_BTN_PIN, &btnDownISR, RISING);

  // ignore returned event handler ids
  WiFi.onEvent(&wiFiProvEventCb);

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  ESP_ERROR_CHECK(esp_event_handler_register(
      ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, any_handler, (void *)0xDEADBEEF));
}