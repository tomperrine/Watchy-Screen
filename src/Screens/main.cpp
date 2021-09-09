
#include "TimeScreen.h"
#include "Events.h"

TimeScreen timeScreen;

void setup() {
  LOGD();  // fail if debugging macros not defined

  start_event_handler();

}

void loop() {
  yield();
 }