#include "CarouselScreen.h"

#include "Watchy.h"

using namespace Watchy;

RTC_DATA_ATTR int8_t CarouselScreen::index;

CarouselScreen::CarouselScreen(CarouselItem *cis, const int8_t cs, uint16_t bg) : Screen(bg), items(cis), size(cs) {
  for (int i = 0; i < size; i++) {
    if (items[i].splash != nullptr) {
      items[i].splash->parent = this;
    }
    if (items[i].child != nullptr) {
      items[i].child->parent = this;
    }
  }
}

void CarouselScreen::show() {
  LOGI("%08x index %d", (uint32_t) this, index);
  Watchy::showWatchFace(true, items[index].splash);
}

void CarouselScreen::menu() {
  LOGI(); 
  if (items[index].child != nullptr) {
    Watchy::setScreen(items[index].child);
  }
}

void CarouselScreen::back() {
  LOGI(); 
  index = 0;
  show();
}

void CarouselScreen::up() {
  LOGI(); 
  index--;
  if (index < 0) {
    index = size - 1;
  }
  show();
}

void CarouselScreen::down() {
  LOGI(); 
  index++;
  if (index >= size) {
    index = 0;
  }
  show();
}
