#pragma once
/*
 * MeshDeck hardware layer for the LilyGo T-Deck:
 * ST7789 320x240 display (PSRAM framebuffer), I2C keyboard, trackball,
 * GT911 touch, backlight PWM, I2S speaker beeps, SD card firmware update.
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ---- T-Deck pins ----
#define TDECK_TFT_CS     12
#define TDECK_TFT_DC     11
#define TDECK_TFT_BL     42
#define TDECK_SPI_SCK    40
#define TDECK_SPI_MISO   38
#define TDECK_SPI_MOSI   41
#define TDECK_SD_CS      39
#define TDECK_KB_ADDR    0x55
#define TDECK_I2C_SDA    18
#define TDECK_I2C_SCL    8
#define TDECK_TOUCH_ADDR 0x5D   // GT911 (alt 0x14)
#define TDECK_TOUCH_INT  16
#define TDECK_TB_UP      3
#define TDECK_TB_DOWN    15
#define TDECK_TB_LEFT    1
#define TDECK_TB_RIGHT   2
#define TDECK_TB_PRESS   0
#define TDECK_I2S_BCK    7
#define TDECK_I2S_WS     5
#define TDECK_I2S_DOUT   6

// ---- input events ----
enum NavEvent : uint8_t {
  NAV_NONE = 0, NAV_UP, NAV_DOWN, NAV_LEFT, NAV_RIGHT, NAV_SELECT, NAV_BACK
};

struct TouchEvent {
  enum Kind : uint8_t { NONE = 0, TAP, DRAG, RELEASE } kind;
  int16_t x, y;      // current position (screen coords)
  int16_t dx, dy;    // delta since last event (DRAG)
};

class DeckHW {
public:
  bool begin(bool flip_display);            // display+input up; true if touch found
  void setRotationFlip(bool flip);

  // -- display --
  GFXcanvas16& cv() { return *_canvas; }
  void push();                              // blit canvas to panel
  void setBacklight(uint8_t level_0_255);
  uint8_t backlight() const { return _bl_level; }
  void displayOff();
  void displayOn();
  bool isDisplayOn() const { return _disp_on; }

  // -- input --
  uint8_t readKey();                        // 0 = none, else ASCII (0x0D enter, 0x08 bksp)
  NavEvent readNav();                       // trackball + button (SELECT / BACK on long press)
  bool readTouch(TouchEvent& ev);
  void setTouchMap(uint8_t m) { _touch_map = m; }   // 0..3, see readTouch()
  bool hasTouch() const { return _touch_addr != 0; }
  uint32_t lastActivityMillis() const { return _last_activity; }
  void kickActivity() { _last_activity = millis(); }

  // -- sound --
  void setSound(bool on, uint8_t volume_0_10) { _snd_on = on; _snd_vol = volume_0_10; }
  void beep(uint16_t freq, uint16_t ms);    // blocking, short
  void chimeMessage();
  void chimeBoot();
  void chimeError();

  // -- SD card (shared SPI bus; call sdBegin, use SD, then sdEnd) --
  bool sdBegin();
  void sdEnd();

  // -- SD firmware update --
  // looks for /firmware.bin on the SD card and flashes it; only returns on failure
  const char* updateFromSD();

private:
  void tbISRUpdate();
  bool gt911Read(uint8_t* buf);
  void i2sTone(uint16_t freq, uint16_t ms);

  SPIClass* _spi = nullptr;                 // HSPI shared bus (display + SD)
  Adafruit_ST7789* _tft = nullptr;
  GFXcanvas16* _canvas = nullptr;
  uint8_t _touch_addr = 0;
  uint8_t _touch_map = 0;
  bool _flip = false;
  bool _disp_on = true;
  uint8_t _bl_level = 255;
  bool _snd_on = true;
  uint8_t _snd_vol = 6;
  uint32_t _last_activity = 0;

  // trackball state
  volatile static int16_t _tb_x, _tb_y;
  static void IRAM_ATTR isrUp();
  static void IRAM_ATTR isrDown();
  static void IRAM_ATTR isrLeft();
  static void IRAM_ATTR isrRight();
  uint32_t _btn_down_at = 0;
  bool _btn_was_down = false;
  bool _btn_long_fired = false;

  // touch state
  bool _touching = false;
  int16_t _tx = 0, _ty = 0, _t_start_x = 0, _t_start_y = 0;
  uint32_t _t_start_ms = 0;
  bool _t_moved = false;
};
