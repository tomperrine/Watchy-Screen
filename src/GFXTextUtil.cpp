#include "GFXTextUtil.h"

#include <Adafruit_GFX.h>
#include <ctype.h>

#include "config.h" // DEBUG

/**
 * @brief when printing, how much width on the screen does this glyph require
 *
 * @param c char to compute width
 * @param f font in use
 * @return int16_t "width" of this character (actual width + inter char spacing)
 */
int16_t charWidth(const char c, const GFXfont *f) {
  if (c < f->first) {
    return 0;
  }
  if (c > f->last) {
    return 0;
  }
  const GFXglyph g = f->glyph[c - f->first];
  return g.xAdvance;
}

/**
 * @brief how far does this glyph extend, relative to the baseline?
 *
 * @param c character to compute
 * @param f font in use
 * @return int16_t how far above or below the baseline does this char extend?
 */
int16_t charDescent(const char c, const GFXfont *f) {
  if (c < f->first) {
    return 0;
  }
  if (c > f->last) {
    return 0;
  }
  const GFXglyph g = f->glyph[c - f->first];
  return g.yOffset + g.height;
}

/**
 * @brief draw word wrapped text within a bounding box. Takes GFX, bounding box,
 *        text string, and font
 *
 * @param g Adafruit_GFX to draw on
 * @param x upper left of bounding box (left)
 * @param y upper left of bounding box (top)
 * @param w width of bounding box
 * @param h height of bounding box
 * @param t text to draw. will draw as much as fits
 * @param f font to use
 */
void drawWordWrappedText(Adafruit_GFX &g, int16_t x, int16_t y, int16_t w,
                         int16_t h, const char *t, const GFXfont *f) {
  if (!t) {
    return;
  }
  g.setFont(f);
  g.setTextWrap(false);  // clip (GFX used to have a bug that would auto-wrap on
                         // exact width)
  const char *startLine = t;
  const char *wordBreak = t;
  int16_t wordBreakXPos = 0;
  int16_t xPos = 0;  ///< x position relative to left of bounding box
  int16_t yPos = f->yAdvance;  ///< y position relative to top of bounding box
  bool inWord = false;
  for (char c = *t;; c = *++t) {
    LOGD("%3d %3d %3d %2d %1c %02x", w, wordBreakXPos, xPos, charWidth(c, f), c, c);
    if (c != '\0' && c != '\n') {
      if (inWord != !isspace(c)) {
        wordBreak = t;
        wordBreakXPos = xPos;
        inWord = !inWord;
      }
      xPos += charWidth(c, f);
      if (xPos <= w) {
        if (yPos + charDescent(c, f) > h) {
          // text won't fit in bounding box. Done.
          return;
        }
        continue;
      }
      // char won't fit on line, fall through
    }
    // wrap
    if (startLine == wordBreak) {
      // word doesn't fit on line. Hard wrap
      wordBreak = t-1;
      wordBreakXPos = xPos-charWidth(c, f);
    }
    g.setCursor(x, y + yPos);
    for (; startLine < wordBreak; startLine++) {
      g.print(*startLine);
    }
    if (!inWord) {
      for (; *t && isSpace(*t) && *t != '\n'; t++) { }
      // consume one leading '\n' since we just started a line
      if (*t == '\n') { t++; }
      if (*t == '\0') { return; }
      startLine = t;
      wordBreak = t;
      inWord = !isspace(*t);
    }
    xPos -= wordBreakXPos;
    yPos += f->yAdvance;
    wordBreakXPos = 0;
  }
}
