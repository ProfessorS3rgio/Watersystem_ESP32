#ifndef BMP_DISPLAY_H
#define BMP_DISPLAY_H

#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// BMP helper functions
uint16_t read16(File &f);
uint32_t read32(File &f);

// Function to draw BMP image from SD card
void drawBMP(const char *filename, int16_t x, int16_t y) {
  File bmpFile;
  int bmpWidth, bmpHeight;
  uint8_t bmpDepth;
  uint32_t bmpImageoffset;
  uint32_t rowSize;
  boolean goodBmp = false;
  boolean flip = true;
  int w, h, row, col;
  uint8_t r, g, b;
  uint32_t pos = 0;

  if ((x >= tft.width()) || (y >= tft.height())) return;

  bmpFile = SD.open(filename);
  if (!bmpFile) {
    Serial.print(F("BMP not found: "));
    Serial.println(filename);
    return;
  }

  // Parse BMP header
  if (read16(bmpFile) == 0x4D42) { // BMP signature
    (void)read32(bmpFile); // File size
    (void)read32(bmpFile); // Creator bytes
    bmpImageoffset = read32(bmpFile);
    (void)read32(bmpFile); // Header size
    bmpWidth = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    
    if (read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile);
      if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed
        goodBmp = true;

        rowSize = (bmpWidth * 3 + 3) & ~3;
        if (bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip = false;
        }

        w = bmpWidth;
        h = bmpHeight;
        if ((x + w - 1) >= tft.width()) w = tft.width() - x;
        if ((y + h - 1) >= tft.height()) h = tft.height() - y;

        for (row = 0; row < h; row++) {
          if (flip)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else
            pos = bmpImageoffset + row * rowSize;
          
          if (bmpFile.position() != pos) {
            bmpFile.seek(pos);
          }

          for (col = 0; col < w; col++) {
            b = bmpFile.read();
            g = bmpFile.read();
            r = bmpFile.read();
            tft.drawPixel(x + col, y + row, tft.color565(r, g, b));
          }
        }
      }
    }
  }

  bmpFile.close();
  if (!goodBmp) Serial.println(F("BMP format not recognized."));
}

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read();
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read();
  return result;
}

#endif // BMP_DISPLAY_H