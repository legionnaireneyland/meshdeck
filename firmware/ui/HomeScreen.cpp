#include "AllScreens.h"
#include "../MyMesh.h"
#include <RTClib.h>

struct AppDef { const char* label; ScreenId scr; uint16_t color; char glyph; };
static const AppDef APPS[9] = {
  { "Chat",      SCR_CHAT,      C_ACCENT, 'C' },
  { "Contacts",  SCR_CONTACTS,  C_GREEN,  '@' },
  { "Map",       SCR_MAP,       C_CYAN,   'M' },
  { "Heard",     SCR_LASTHEARD, C_YELLOW, 'H' },
  { "Repeaters", SCR_REPEATERS, C_ORANGE, 'R' },
  { "Trace",     SCR_TRACE,     C_PURPLE, 'T' },
  { "Noise",     SCR_NOISE,     C_PINK,   'N' },
  { "Terminal",  SCR_TERMINAL,  C_FG,     '>' },
  { "Settings",  SCR_SETTINGS,  C_FG_DIM, 'S' },
};

// grid layout
#define GRID_X0   14
#define GRID_Y0   96
#define CELL_W    100
#define CELL_H    46

void HomeScreen::draw() {
  GFXcanvas16& c = ui.cv();
  c.fillScreen(C_BG);

  // big clock
  char clk[8];
  ui.fmtClock(clk, sizeof(clk));
  c.setTextSize(5);
  c.setTextColor(C_FG);
  c.setCursor(84, 16);
  c.print(clk);

  // date + node name
  uint32_t e = ui.epochNow();
  c.setTextSize(1);
  c.setTextColor(C_FG_DIM);
  if (e > 1000000000) {
    DateTime dt(e);
    static const char* DOW[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    char d[40];
    snprintf(d, sizeof(d), "%s %02d/%02d/%04d", DOW[dt.dayOfTheWeek()], dt.day(), dt.month(), dt.year());
    c.setCursor(SCREEN_W / 2 - strlen(d) * 3, 62);
    c.print(d);
  }
  char nn[48];
  snprintf(nn, sizeof(nn), "%s  |  %d unread", ui.prefs ? ui.prefs->node_name : "?", ui.store.totalUnread());
  c.setCursor(SCREEN_W / 2 - strlen(nn) * 3, 76);
  c.setTextColor(C_FG_FAINT);
  c.print(nn);

  // battery + mesh in corners
  char b[16];
  int pct = ui.batteryPercent();
  if (pct >= 0) {
    snprintf(b, sizeof(b), "%d%%", pct);
    c.setTextColor(pct > 20 ? C_FG_DIM : C_RED);
    c.setCursor(SCREEN_W - 8 - strlen(b) * 6, 8);
    c.print(b);
  }
  int bars = ui.meshBars();
  for (int i = 0; i < 4; i++) {
    int bh = 3 + i * 3;
    c.fillRect(8 + i * 5, 20 - bh, 3, bh, i < bars ? C_ACCENT : C_FG_FAINT);
  }
  c.setTextColor(C_FG_FAINT);
  c.setCursor(8, 26);
  c.print("mesh");

  // app grid 3x3
  for (int i = 0; i < 9; i++) {
    int gx = GRID_X0 + (i % 3) * CELL_W;
    int gy = GRID_Y0 + (i / 3) * CELL_H;
    bool sel = i == _sel;
    c.fillRoundRect(gx, gy, CELL_W - 8, CELL_H - 6, 8, sel ? C_BG_RAISED : C_BG_ALT);
    if (sel) c.drawRoundRect(gx, gy, CELL_W - 8, CELL_H - 6, 8, APPS[i].color);
    // glyph badge
    c.fillRoundRect(gx + 6, gy + 8, 24, 24, 6, APPS[i].color);
    c.setTextSize(2);
    c.setTextColor(C_BG);
    c.setCursor(gx + 12, gy + 13);
    c.write(APPS[i].glyph);
    c.setTextSize(1);
    c.setTextColor(sel ? C_FG : C_FG_DIM);
    c.setCursor(gx + 36, gy + 16);
    c.print(APPS[i].label);
    // unread badge on chat
    if (APPS[i].scr == SCR_CHAT) {
      int u = ui.store.totalUnread();
      if (u > 0) {
        c.fillCircle(gx + CELL_W - 18, gy + 8, 7, C_RED);
        c.setTextColor(0xFFFF);
        c.setCursor(gx + CELL_W - 18 - (u > 9 ? 6 : 3), gy + 5);
        c.print(u > 99 ? 99 : u);
      }
    }
  }
}

bool HomeScreen::key(uint8_t k) {
  if (k >= '1' && k <= '9') {
    ui.go(APPS[k - '1'].scr);
    return true;
  }
  if (k == 0x0D) { ui.go(APPS[_sel].scr); return true; }
  return false;
}

bool HomeScreen::nav(NavEvent e) {
  switch (e) {
    case NAV_UP:    if (_sel >= 3) _sel -= 3; return true;
    case NAV_DOWN:  if (_sel < 6) _sel += 3; return true;
    case NAV_LEFT:  if (_sel % 3) _sel--; return true;
    case NAV_RIGHT: if (_sel % 3 < 2) _sel++; return true;
    case NAV_SELECT: ui.go(APPS[_sel].scr); return true;
    case NAV_BACK:
      // already at home - do nothing (never blank the screen here)
      return true;
    default: return false;
  }
}

bool HomeScreen::touch(const TouchEvent& e) {
  if (e.kind != TouchEvent::TAP) return false;
  if (e.y < GRID_Y0) return true;
  int col = (e.x - GRID_X0) / CELL_W;
  int row = (e.y - GRID_Y0) / CELL_H;
  if (col >= 0 && col < 3 && row >= 0 && row < 3) {
    _sel = row * 3 + col;
    ui.go(APPS[_sel].scr);
  }
  return true;
}
