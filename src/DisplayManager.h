#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <TFT_eSPI.h>

class DisplayManager {
 public:
  static DisplayManager& getInstance() {
    static DisplayManager instance;
    return instance;
  }

  bool begin() {
    tft.init();
    tft.setRotation(0);  // Portrait mode (was Landscape)
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    // Create sprite buffer
    sprite.createSprite(tft.width(), tft.height());
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    return true;
  }

  void clearScreen() {
    sprite.fillSprite(TFT_BLACK);
    sprite.setCursor(0, 0);
  }

  void drawText(const char* text, int x, int y, uint8_t size = 1) {
    sprite.setTextSize(size);
    sprite.drawString(text, x, y);
  }

  void pushToDisplay() { sprite.pushSprite(0, 0); }

  TFT_eSPI* getTFT() { return &tft; }

 private:
  DisplayManager() = default;
  TFT_eSPI tft;
  TFT_eSprite sprite = TFT_eSprite(&tft);
};

#endif