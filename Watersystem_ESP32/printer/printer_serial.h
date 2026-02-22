#ifndef PRINTER_SERIAL_H
#define PRINTER_SERIAL_H

#include <HardwareSerial.h>
#include "../configuration/config.h"  // for PRINTER_RX/TX and PRINTER_BAUD

// Mirror the sample's RXD2/TXD2 names for compatibility
#define RXD2 PRINTER_RX
#define TXD2 PRINTER_TX

class ThermalPrinter : public Print {
public:
    ThermalPrinter(HardwareSerial& hw) : _serial(hw) {}

    // initialize the underlying UART (call once during setup)
    void begin(long baud = PRINTER_BAUD) {
        _serial.begin(baud, SERIAL_8N1, RXD2, TXD2);
    }

    // wake and reset printer
    void wake() {
        // ESC @
        _serial.write(0x1B);
        _serial.write('@');
    }

    // set defaults (mimic Adafruit_Thermal)
    void setDefault() {
        wake();
        setLineSpacing(30);
    }

    // adjust line spacing (ESC 3 n)
    void setLineSpacing(uint8_t spacing) {
        _serial.write(0x1B);
        _serial.write('3');
        _serial.write(spacing);
    }

    // low‑power sleep (no op for now)
    void sleep() {
        // not all printers support this; sending a generic sleep command
        _serial.write(0x1B);
        _serial.write('8');
    }

    // text justification: 'L', 'C', 'R'
    void justify(char c) {
        uint8_t val = 0;
        if (c == 'C') val = 1;
        else if (c == 'R') val = 2;
        _serial.write(0x1B);
        _serial.write('a');
        _serial.write(val);
    }

    // character size: 'S' small/normal, 'M' medium (double height), 'L' large (double width)
    void setSize(char s) {
        uint8_t n = 0;
        switch (s) {
            case 'M': n = (1 << 4); break;   // double height
            case 'L': n = (1 << 3); break;   // double width
            default:  n = 0;                 // normal
        }
        _serial.write(0x1D);
        _serial.write('!');
        _serial.write(n);
    }

    void boldOn() {
        _serial.write(0x1B);
        _serial.write('E');
        _serial.write(1);
    }
    void boldOff() {
        _serial.write(0x1B);
        _serial.write('E');
        _serial.write(0);
    }

    // simple bitmap dump using GS v 0 (same as sample code)
    void printBitmap(int width, int height, const uint8_t *bitmap) {
        if (width <= 0 || height <= 0) return;
        int bytesPerLine = (width + 7) / 8;
        int totalChunks = (height + 23) / 24;

        // temporarily set line spacing to 0 so that we can dump raw rows
        _serial.write(0x1B);
        _serial.write('3');
        _serial.write((uint8_t)0);

        for (int chunk = 0; chunk < totalChunks; chunk++) {
            int linesInChunk = min(24, height - chunk * 24);
            uint8_t yL = linesInChunk & 0xFF;
            uint8_t yH = (linesInChunk >> 8) & 0xFF;
            uint8_t xL = bytesPerLine & 0xFF;
            uint8_t xH = (bytesPerLine >> 8) & 0xFF;

            _serial.write(0x1D);
            _serial.write('v');
            _serial.write('0');
            _serial.write((uint8_t)0);
            _serial.write(xL);
            _serial.write(xH);
            _serial.write(yL);
            _serial.write(yH);

            for (int row = 0; row < linesInChunk; row++) {
                int baseIndex = (chunk * 24 + row) * bytesPerLine;
                for (int col = 0; col < bytesPerLine; col++) {
                    _serial.write(bitmap[baseIndex + col]);
                }
            }
        }

        // restore default line spacing used by the rest of the code
        setLineSpacing(30);
    }

    // provide Print API
    size_t write(uint8_t c) override { return _serial.write(c); }
    // Print already provides a char* overload, no need to reimplement
    //size_t write(const char *str) override { return _serial.write(str); }
    size_t write(const uint8_t *buffer, size_t size) override { return _serial.write(buffer, size); }

    // convenience helper used by power_save_manager
    void feed(uint8_t lines = 1) {
        _serial.write(0x1B);
        _serial.write('d');
        _serial.write(lines);
        delay(10);
    }

private:
    HardwareSerial& _serial;
};

// global printer instance will be defined in the main .ino
extern ThermalPrinter printer;

#endif // PRINTER_SERIAL_H
