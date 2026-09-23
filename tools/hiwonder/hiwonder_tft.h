#pragma once
// Hiwonder vendor ST7789 profile, reused from the working Bruce/Marauder port.
#define USER_SETUP_LOADED
#define USER_SETUP_INFO "Hiwonder ESP32-S3"
#define ST7789_2_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_MISO -1
#define TFT_MOSI 47
#define TFT_SCLK 21
#define TFT_CS 2
#define TFT_DC 3
#define TFT_RST -1
#define USE_HSPI_PORT
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

