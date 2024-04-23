#pragma once

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "envreader.h"

// ----------------------------
// Touch Screen pins
// ----------------------------

// The CYD touch uses some non default
// SPI pins

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// ----------------------------

#define FONT_SIZE 4
#define LINE_HEIGHT 32

class Display {
   protected:
    SPIClass spi = SPIClass(VSPI);
    XPT2046_Touchscreen ts = XPT2046_Touchscreen(XPT2046_CS, XPT2046_IRQ);
    TFT_eSPI tft = TFT_eSPI();
    unsigned long t0 = 0;

    EnvReader &envReader;
    int lastCounter = 0;

   public:
    Display(EnvReader &envReader) : envReader(envReader) {}

    void init() {
        spi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
        ts.begin(spi);
        ts.setRotation(1);

        // Start the tft display and set it to black
        tft.init();
        tft.invertDisplay(true);
        tft.setRotation(1);  // This is the display in landscape

        tft.fillScreen(TFT_BLACK);
    }

    void tick() {
        unsigned long t1 = millis();
        long delta = t1 - t0;

        if (delta < 0) {
            delta = 0xFFFFFFFF - t0 + t1;
        }

        if (delta < 100) {
            return;
        }

        t0 = t1;

        if (lastCounter == envReader.counter) return;

        lastCounter = envReader.counter;

        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        auto maxX = tft.drawString("Temp:", 10, 10, FONT_SIZE);
        maxX = max(tft.drawString("Humid:", 10, 10 + LINE_HEIGHT, FONT_SIZE),
                   maxX);
        maxX = max(tft.drawString("AQI:", 10, 10 + LINE_HEIGHT * 2, FONT_SIZE),
                   maxX);
        maxX = max(tft.drawString("VOC:", 10, 10 + LINE_HEIGHT * 3, FONT_SIZE),
                   maxX);
        maxX = max(tft.drawString("CO2:", 10, 10 + LINE_HEIGHT * 4, FONT_SIZE),
                   maxX);

        tft.drawFloat(envReader.temperature, 2, 20 + maxX, 10, FONT_SIZE);
        tft.drawFloat(envReader.humidity, 2, 20 + maxX, 10 + LINE_HEIGHT,
                      FONT_SIZE);
        tft.drawString(envReader.aqi(), 20 + maxX, 10 + LINE_HEIGHT * 2,
                       FONT_SIZE);
        tft.drawNumber(envReader.tvoc, 20 + maxX, 10 + LINE_HEIGHT * 3,
                       FONT_SIZE);
        tft.drawNumber(envReader.eco2, 20 + maxX, 10 + LINE_HEIGHT * 4,
                       FONT_SIZE);
    }
};
