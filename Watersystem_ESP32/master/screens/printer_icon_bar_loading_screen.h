#ifndef PRINTER_ICON_BAR_LOADING_SCREEN_H
#define PRINTER_ICON_BAR_LOADING_SCREEN_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Combined loading screen: centered printer icon with a centered progress bar below.
// Call printerIconBarScreenUpdate(...) frequently from loop().

struct PrinterIconBarLoadingState {
  uint32_t lastFrameMs = 0;
  bool drawn = false;

  // Printer animation
  int step = 0;

  // Bar animation
  int lastProgress = -1;
  int animStep = 0;

  // Cached layout
  int iconX = 0;
  int iconY = 0;
  int barX = 0;
  int barY = 0;
};

static inline int clampPercent2(int v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return v;
}

static inline void printerIconBarDrawStatic(TFT_eSPI &tft, int w, int h,
                                           uint16_t bg, uint16_t header, uint16_t text, uint16_t line) {
  tft.fillScreen(bg);

  // Accent bars
  tft.fillRect(0, 0, w, 5, TFT_BLUE);
  tft.fillRect(0, h - 5, w, 5, TFT_BLUE);

  // Title
  tft.setTextColor(header, bg);
  tft.setTextSize(1);
  tft.setCursor(18, 18);
  tft.print(F("PRINTING RECEIPT"));

  tft.drawLine(0, 30, w, 30, line);

  // Subtext
  tft.setTextColor(text, bg);
  tft.setTextSize(1);
  tft.setCursor(18, 42);
  tft.print(F("Please wait. Do not power off."));
}

static inline void printerIconBarDrawPrinter(TFT_eSPI &tft,
                                            int x, int y, int w, int h,
                                            int paperOffset, bool lightOn,
                                            uint16_t bg, uint16_t line, uint16_t amount) {
  // Clear only the paper travel area to avoid flicker
  const int paperTravel = 20;
  int clearX = x + 23;
  int clearY = y + 4;
  int clearW = w - 46;
  int clearH = 32 + paperTravel;
  tft.fillRect(clearX, clearY, clearW, clearH, bg);

  // Paper (animated)
  int paperW = w - 46;
  int paperH = 32;
  int paperX = x + 23;
  int paperY = y + 4 + paperOffset;
  tft.fillRoundRect(paperX, paperY, paperW, paperH, 4, TFT_WHITE);
  tft.drawRoundRect(paperX, paperY, paperW, paperH, 4, line);

  // Paper lines
  for (int i = 0; i < 3; i++) {
    int ly = paperY + 10 + i * 7;
    tft.drawLine(paperX + 8, ly, paperX + paperW - 8, ly, TFT_CYAN);
  }

  // Printer body
  int bodyX = x + 12;
  int bodyY = y + 24;
  int bodyW = w - 24;
  int bodyH = h - 30;
  tft.fillRoundRect(bodyX, bodyY, bodyW, bodyH, 10, TFT_BLACK);
  tft.drawRoundRect(bodyX, bodyY, bodyW, bodyH, 10, line);

  // Output slot
  tft.fillRoundRect(bodyX + 14, bodyY + 14, bodyW - 28, 8, 4, line);

  // Status light
  uint16_t lightColor = lightOn ? amount : TFT_DARKGREY;
  tft.fillCircle(bodyX + bodyW - 18, bodyY + bodyH - 16, 6, lightColor);
  tft.drawCircle(bodyX + bodyW - 18, bodyY + bodyH - 16, 6, line);
}

static inline void printerIconBarComputeLayout(PrinterIconBarLoadingState &state, int screenW, int screenH,
                                              int iconW, int iconH,
                                              int barW, int barH) {
  const int headerBottom = 66; // title + subtext area
  const int bottomMargin = 14;
  const int gap = 14;

  int contentTop = headerBottom;
  int contentH = iconH + gap + barH + 34; // includes percent/done text lines
  int availableH = screenH - contentTop - bottomMargin;
  int offsetY = (availableH - contentH) / 2;
  if (offsetY < 0) offsetY = 0;

  state.iconX = (screenW - iconW) / 2;
  state.iconY = contentTop + offsetY;

  state.barX = (screenW - barW) / 2;
  state.barY = state.iconY + iconH + gap;
}

static inline void printerIconBarScreenBegin(TFT_eSPI &tft, PrinterIconBarLoadingState &state,
                                            int screenW, int screenH,
                                            uint16_t bg, uint16_t header, uint16_t text, uint16_t line,
                                            uint16_t amount) {
  printerIconBarDrawStatic(tft, screenW, screenH, bg, header, text, line);

  const int iconW = 130;
  const int iconH = 120;
  const int barW = 240;
  const int barH = 22;
  printerIconBarComputeLayout(state, screenW, screenH, iconW, iconH, barW, barH);

  state.drawn = true;
  state.step = 0;
  state.lastFrameMs = 0;
  state.lastProgress = -1;
  state.animStep = 0;

  // Establish the printer body
  printerIconBarDrawPrinter(tft, state.iconX, state.iconY, iconW, iconH, 0, false, bg, line, amount);
}

static inline void printerIconBarScreenUpdate(TFT_eSPI &tft, PrinterIconBarLoadingState &state,
                                             int screenW, int screenH,
                                             int progress, bool done,
                                             uint16_t bg, uint16_t header, uint16_t text, uint16_t line, uint16_t amount) {
  if (!state.drawn) {
    printerIconBarScreenBegin(tft, state, screenW, screenH, bg, header, text, line, amount);
  }

  const uint32_t now = millis();
  if (now - state.lastFrameMs < 80) return;
  state.lastFrameMs = now;

  // --- Printer animation ---
  int i = state.step;
  int paperOffset = (i / 3) % 20;
  bool lightOn = (i / 12) % 2;

  const int iconW = 130;
  const int iconH = 120;
  printerIconBarDrawPrinter(tft, state.iconX, state.iconY, iconW, iconH, paperOffset, lightOn, bg, line, amount);

  // --- Bar animation ---
  int pct = clampPercent2(progress);
  if (done) pct = 100;

  const int barW = 240;
  const int barH = 22;
  const int pad = 2;
  const int innerW = barW - pad * 2;
  const int innerH = barH - pad * 2;

  // Border
  tft.drawRoundRect(state.barX, state.barY, barW, barH, 6, line);

  // Interior
  tft.fillRoundRect(state.barX + pad, state.barY + pad, innerW, innerH, 5, bg);
  int fillW = (innerW * pct) / 100;
  if (fillW > 0) {
    tft.fillRoundRect(state.barX + pad, state.barY + pad, fillW, innerH, 5, amount);

    // moving highlight
    int highlightW = 18;
    int hx = state.barX + pad + (state.animStep % (innerW + highlightW)) - highlightW;
    int hx0 = max(hx, state.barX + pad);
    int hx1 = min(hx + highlightW, state.barX + pad + fillW);
    if (hx1 > hx0) {
      tft.fillRect(hx0, state.barY + pad, hx1 - hx0, innerH, TFT_DARKGREY);
    }
  }

  // Text below bar
  if (pct != state.lastProgress) {
    state.lastProgress = pct;
    tft.fillRect(state.barX, state.barY + barH + 10, barW, 28, bg);

    tft.setTextColor(text, bg);
    tft.setTextSize(1);
    tft.setCursor(state.barX, state.barY + barH + 10);
    tft.print(F("Progress: "));
    tft.print(pct);
    tft.print(F("%"));
  }

  if (done) {
    tft.setTextColor(amount, bg);
    tft.setTextSize(1);
    tft.setCursor(state.barX, state.barY + barH + 24);
    tft.print(F("Done!"));
  }

  state.step++;
  state.animStep += 6;

  (void)screenW;
  (void)screenH;
}

#endif  // PRINTER_ICON_BAR_LOADING_SCREEN_H
