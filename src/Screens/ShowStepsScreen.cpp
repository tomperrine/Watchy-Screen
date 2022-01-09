#include "ShowStepsScreen.h"

#include "OptimaLTStd22pt7b.h"
#include "Watchy.h"

void ShowStepsScreen::show() {
  Watchy::RTC.setRefresh(RTC_REFRESH_MIN);
  Watchy::display.fillScreen(bgColor);
  Watchy::display.setFont(OptimaLTStd22pt7b);
  Watchy::display.printf("\n%d\nsteps", Watchy::sensor.getCounter());
}
