
#include "TimeScreen.h"
#include "Events.h"

TimeScreen timeScreen;

void setup() {
  LOGD();  // fail if debugging macros not defined

  Watchy_Event::start();

}

void loop() {
  yield();
 }