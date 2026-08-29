#include <Arduino.h>
#include <SPI.h>
#include <Arduino_GFX_Library.h>
#include <qrcode.h>

/* ============================================================
 * Waveshare ESP32-S3-LCD-2 (ST7789 240x320 IPS) Pin Definitions
 * ============================================================ */
#define LCD_SCLK 39
#define LCD_MOSI 38
#define LCD_MISO 40
#define LCD_DC   42
#define LCD_CS   45
#define LCD_RST  -1   // Software reset
#define LCD_BL   1    // Backlight GPIO (PWM capable)

// Hardware Accelerated SPI Bus & ST7789 IPS Display (80MHz)
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO, FSPI);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0 /* Portrait (240x320) */, true /* IPS */, 240, 320);

// Configuration
const char* BIRTHDAY_GIRL = "Reshma";
const char* FULL_NAME     = "Reshma Ramachandran";
const char* WEBSITE_URL   = "https://reshma.nayisw.tech";

// 16-bit RGB565 Color Palette (Curated Luxury Aesthetic)
#define COLOR_BG          0x0821  // Midnight Velvet Navy
#define COLOR_CARD_BG     0x10A5  // Deep Glass Card
#define COLOR_GOLD        0xFE60  // Brilliant Radiant Gold
#define COLOR_GOLD_DARK   0x9B80  // Antique Bronze Gold
#define COLOR_GOLD_AURA   0x5280  // Soft Gold Glow
#define COLOR_ROSE        0xFBCF  // Radiant Rose Gold
#define COLOR_PINK_DEEP   0xD9A9  // Elegant Blush Magenta
#define COLOR_WHITE       0xFFFF  // Crisp Pure White
#define COLOR_MUTED       0xAD75  // Slate Silver
#define COLOR_HEART_RED   0xF843  // Crimson Ruby
#define COLOR_HEART_AURA  0x6800  // Soft Ruby Halo
#define COLOR_CAKE_SPONGE 0xFDE7  // Vanilla Cream Sponge
#define COLOR_CAKE_FROST  0xFC74  // Strawberry Pink Frosting
#define COLOR_FLAME_CORE  0xFFFF  // Pure White-Yellow Core
#define COLOR_FLAME_MID   0xFE20  // Radiant Amber
#define COLOR_FLAME_AURA  0x8A00  // Soft Fiery Glow
#define COLOR_SMOKE       0x9CD3  // Soft Gray Smoke

/* ============================================================
 * Precision Typography & Anti-Aliased Geometric Helpers
 * ============================================================ */

// Calculate width with custom letter-spacing
int getTextWidthSpaced(const char* text, uint8_t size, int letterSpacing) {
  int len = strlen(text);
  if (len == 0) return 0;
  int charW = size * 6;
  return (len * charW) + ((len - 1) * letterSpacing);
}

// Subpixel Centered Spaced Text with Soft Drop-Shadow
void drawSpacedText(const char* text, int y, uint8_t size, int letterSpacing, uint16_t color, uint16_t shadowColor = 0x0000, uint16_t bg = COLOR_BG) {
  int totalW = getTextWidthSpaced(text, size, letterSpacing);
  int startX = (240 - totalW) / 2;
  if (startX < 2) startX = 2;

  int charW = size * 6;
  int curX = startX;
  gfx->setTextSize(size);

  const char* p = text;
  while (*p) {
    char ch[2] = { *p, '\0' };
    if (shadowColor != 0x0000) {
      gfx->setTextColor(shadowColor, bg);
      gfx->setCursor(curX + 1, y + 1);
      gfx->print(ch);
    }
    gfx->setTextColor(color, bg);
    gfx->setCursor(curX, y);
    gfx->print(ch);

    curX += charW + letterSpacing;
    p++;
  }
}

// Subpixel Centered Text on Screen (Width = 240) using GFX Text Bounds
void drawCenteredText(const char* text, int y, uint8_t size, uint16_t color, uint16_t bg = COLOR_BG) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(size);
  gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (240 - (int)w) / 2 - x1;
  if (x < 2) x = 2;
  gfx->setTextColor(color, bg);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// Subpixel Centered Text inside any custom Box Bounds
void drawCenteredTextInBox(const char* text, int boxX, int boxW, int y, uint8_t size, uint16_t color, uint16_t bg) {
  int16_t x1, y1;
  uint16_t w, h;
  gfx->setTextSize(size);
  gfx->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = boxX + (boxW - (int)w) / 2 - x1;
  gfx->setTextColor(color, bg);
  gfx->setCursor(x, y);
  gfx->print(text);
}

// Vector Royal Crown Icon
void drawCrown(int cx, int cy, uint16_t color) {
  gfx->fillTriangle(cx - 14, cy + 6, cx + 14, cy + 6, cx, cy - 6, color);
  gfx->fillTriangle(cx - 14, cy + 6, cx - 18, cy - 2, cx - 8, cy + 6, color);
  gfx->fillTriangle(cx + 14, cy + 6, cx + 18, cy - 2, cx + 8, cy + 6, color);
  gfx->fillCircle(cx - 18, cy - 2, 2, COLOR_WHITE);
  gfx->fillCircle(cx, cy - 6, 2, COLOR_WHITE);
  gfx->fillCircle(cx + 18, cy - 2, 2, COLOR_WHITE);
  gfx->fillRect(cx - 14, cy + 6, 28, 3, color);
}

// Anti-Aliased Heart with Depth Highlight & Shaded Halo
void drawAntiAliasedHeart(int cx, int cy, int size, uint16_t color, uint16_t aura = COLOR_HEART_AURA) {
  int r = size / 2;
  if (r < 2) r = 2;

  // 1. Soft Outer Shading Halo
  gfx->fillCircle(cx - r / 2, cy - r / 3, r / 2 + 1, aura);
  gfx->fillCircle(cx + r / 2, cy - r / 3, r / 2 + 1, aura);
  gfx->fillTriangle(cx - r - 1, cy - r / 4, cx + r + 1, cy - r / 4, cx, cy + r + 2, aura);

  // 2. Core Solid Heart
  gfx->fillCircle(cx - r / 2, cy - r / 3, r / 2, color);
  gfx->fillCircle(cx + r / 2, cy - r / 3, r / 2, color);
  gfx->fillTriangle(cx - r, cy - r / 4, cx + r, cy - r / 4, cx, cy + r, color);

  // 3. Specular Highlight for Depth
  if (size >= 10) {
    gfx->drawPixel(cx - r / 2 - 1, cy - r / 3 - 1, COLOR_WHITE);
  }
}

// Anti-Aliased Shaded Diamond Star
void drawAntiAliasedSparkle(int cx, int cy, int len, uint16_t coreColor, uint16_t aura = COLOR_GOLD_AURA) {
  // Soft outer rays
  gfx->drawFastHLine(cx - len - 1, cy, (len + 1) * 2 + 1, aura);
  gfx->drawFastVLine(cx, cy - len - 1, (len + 1) * 2 + 1, aura);

  // Core bright rays
  gfx->drawFastHLine(cx - len, cy, len * 2 + 1, coreColor);
  gfx->drawFastVLine(cx, cy - len, len * 2 + 1, coreColor);

  // Anti-aliased diagonal corner falloff
  if (len >= 3) {
    gfx->drawPixel(cx - 1, cy - 1, aura);
    gfx->drawPixel(cx + 1, cy - 1, aura);
    gfx->drawPixel(cx - 1, cy + 1, aura);
    gfx->drawPixel(cx + 1, cy + 1, aura);
  }
  // Diamond white center
  gfx->drawPixel(cx, cy, COLOR_WHITE);
}

// Mathematically Centered Carousel Progress Dots
void drawFooterDots(int activeIdx, int total = 4) {
  int spacing = 20;
  int startX = (240 - ((total - 1) * spacing)) / 2;
  int y = 310;

  for (int i = 0; i < total; i++) {
    int cx = startX + (i * spacing);
    if (i == activeIdx) {
      gfx->fillRoundRect(cx - 8, y - 2, 16, 5, 2, COLOR_GOLD);
    } else {
      gfx->fillCircle(cx, y, 2, 0x39A6);
    }
  }
}

// Draw cute ribbon bow
void drawRibbonBow(int cx, int cy, uint16_t color) {
  // Left loop
  gfx->fillTriangle(cx, cy, cx - 12, cy - 6, cx - 10, cy + 6, color);
  // Right loop
  gfx->fillTriangle(cx, cy, cx + 12, cy - 6, cx + 10, cy + 6, color);
  // Tails
  gfx->fillTriangle(cx - 2, cy + 2, cx - 8, cy + 12, cx - 2, cy + 10, color);
  gfx->fillTriangle(cx + 2, cy + 2, cx + 8, cy + 12, cx + 2, cy + 10, color);
  // Knot
  gfx->fillCircle(cx, cy, 3, COLOR_GOLD);
  gfx->drawPixel(cx - 1, cy - 1, COLOR_WHITE);
}

// Draw cute angelic wings around heart center
void drawAngelWings(int cx, int cy, int size, uint16_t color) {
  // Left wing feathers
  gfx->fillRoundRect(cx - size - 20, cy - 8, 22, 7, 3, color);
  gfx->fillRoundRect(cx - size - 16, cy - 1, 18, 6, 3, color);
  gfx->fillRoundRect(cx - size - 12, cy + 5, 14, 5, 2, color);
  
  // Right wing feathers
  gfx->fillRoundRect(cx + size - 2, cy - 8, 22, 7, 3, color);
  gfx->fillRoundRect(cx + size - 2, cy - 1, 18, 6, 3, color);
  gfx->fillRoundRect(cx + size - 2, cy + 5, 14, 5, 2, color);

  // Soft feather highlights
  gfx->drawFastHLine(cx - size - 18, cy - 7, 16, COLOR_WHITE);
  gfx->drawFastHLine(cx + size, cy - 7, 16, COLOR_WHITE);
}

// Draw cluster of 3 cute baby hearts
void drawHeartCluster(int cx, int cy) {
  drawAntiAliasedHeart(cx - 6, cy + 2, 7, COLOR_PINK_DEEP);
  drawAntiAliasedHeart(cx + 6, cy + 2, 7, COLOR_ROSE);
  drawAntiAliasedHeart(cx, cy - 4, 9, COLOR_HEART_RED);
  drawAntiAliasedSparkle(cx, cy - 4, 3, COLOR_GOLD);
}

/* ============================================================
 * BOOT ANIMATION: Creative Retro-Luxury Pixel Heart Genesis
 * ============================================================ */
void drawPixelArtHeart(int cx, int cy, int pixelSize, uint16_t coreCol, uint16_t borderCol) {
  // 11x10 Pixel Art Heart Matrix (0=empty, 1=border, 2=core, 3=highlight)
  const uint8_t heartMap[10][11] = {
    {0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0},
    {1, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1},
    {1, 3, 3, 2, 1, 0, 1, 2, 2, 2, 1},
    {1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {0, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0},
    {0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}
  };

  int startX = cx - (11 * pixelSize) / 2;
  int startY = cy - (10 * pixelSize) / 2;

  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 11; c++) {
      uint8_t type = heartMap[r][c];
      if (type != 0) {
        uint16_t col = (type == 1) ? borderCol : ((type == 3) ? COLOR_WHITE : coreCol);
        gfx->fillRoundRect(startX + (c * pixelSize), startY + (r * pixelSize), pixelSize - 1, pixelSize - 1, (pixelSize > 5) ? 2 : 1, col);
      }
    }
  }
}

void runBootAnimation() {
  gfx->fillScreen(0x0000);

  // 1. Single Diamond Singularity Awakening
  for (int p = 1; p <= 5; p++) {
    drawAntiAliasedSparkle(120, 125, p, COLOR_WHITE, COLOR_GOLD_AURA);
    delay(30);
  }
  delay(80);

  // 2. Organic Pixel Heart Materialization (Row-by-Row Cascade)
  const uint8_t heartMap[10][11] = {
    {0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0},
    {1, 3, 2, 1, 0, 0, 0, 1, 2, 2, 1},
    {1, 3, 3, 2, 1, 0, 1, 2, 2, 2, 1},
    {1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1},
    {0, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0},
    {0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0}
  };

  int pSize = 8;
  int startX = 120 - (11 * pSize) / 2;
  int startY = 120 - (10 * pSize) / 2;

  // Build row by row from top to bottom
  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 11; c++) {
      uint8_t type = heartMap[r][c];
      if (type != 0) {
        uint16_t col = (type == 1) ? COLOR_GOLD : ((type == 3) ? COLOR_WHITE : COLOR_HEART_RED);
        gfx->fillRoundRect(startX + (c * pSize), startY + (r * pSize), pSize - 1, pSize - 1, 2, col);
      }
    }
    delay(30);
  }

  // 3. Specular Diamond Glint Across the Heart Facets
  for (int d = 0; d < 11; d++) {
    for (int r = 0; r < 10; r++) {
      int c = d - r;
      if (c >= 0 && c < 11 && heartMap[r][c] != 0) {
        gfx->fillRoundRect(startX + (c * pSize), startY + (r * pSize), pSize - 1, pSize - 1, 2, COLOR_WHITE);
      }
    }
    delay(25);
    // Restore colors
    for (int r = 0; r < 10; r++) {
      int c = d - r;
      if (c >= 0 && c < 11 && heartMap[r][c] != 0) {
        uint8_t type = heartMap[r][c];
        uint16_t col = (type == 1) ? COLOR_GOLD : ((type == 3) ? COLOR_WHITE : COLOR_HEART_RED);
        gfx->fillRoundRect(startX + (c * pSize), startY + (r * pSize), pSize - 1, pSize - 1, 2, col);
      }
    }
  }

  // 4. Organic Heartbeat Pulse & Gentle Golden Halo
  for (int beat = 0; beat < 5; beat++) {
    int curSize = (beat % 2 == 0) ? 9 : 8;
    uint16_t core = (beat % 2 == 0) ? COLOR_HEART_RED : COLOR_ROSE;
    
    // Cleanly wipe heart footprint
    gfx->fillRect(60, 60, 120, 120, 0x0000);

    // Soft shockwave halo on beat
    if (beat % 2 == 0) {
      gfx->drawCircle(120, 120, 52, COLOR_GOLD_AURA);
      gfx->drawCircle(120, 120, 54, COLOR_GOLD);
    }

    drawPixelArtHeart(120, 120, curSize, core, COLOR_GOLD);
    delay(260);
  }

  // 5. Clean, Minimal Luxury Typography (No clutter, pure elegance)
  drawSpacedText("RESHMA", 194, 2, 6, COLOR_WHITE, 0x3186, 0x0000);
  drawSpacedText("CHAPTER 20", 222, 1, 3, COLOR_GOLD, 0x0000, 0x0000);
  drawAntiAliasedSparkle(120, 244, 3, COLOR_GOLD);

  delay(900);

  // Optical Flash Transition into Scene 1
  gfx->fillScreen(COLOR_WHITE);
  delay(60);
  gfx->fillScreen(COLOR_BG);
  delay(80);
}

/* ============================================================
 * SCENE 1: Royal Love Castle & Winged Pixel Heart Locket
 * ============================================================ */
void renderScene1_Welcome() {
  gfx->fillScreen(COLOR_BG);
  drawFooterDots(0);

  // 1. Ornate Gold & Sakura Double Frame (Y = 8 to 296, Height = 288)
  gfx->drawRoundRect(8, 8, 224, 290, 10, COLOR_GOLD);
  gfx->drawRoundRect(10, 10, 220, 286, 8, COLOR_PINK_DEEP);

  // 2. Cute Ribbon Bows at Top & Bottom Centers
  drawRibbonBow(120, 8, COLOR_ROSE);
  drawRibbonBow(120, 296, COLOR_ROSE);

  // 3. Cute 4-Corner Heart Clusters
  drawHeartCluster(24, 24);
  drawHeartCluster(216, 24);
  drawHeartCluster(24, 280);
  drawHeartCluster(216, 280);

  // 4. Scalloped Pastel Heart Garland along Top & Bottom
  for (int h = 0; h < 5; h++) {
    int hx = 60 + (h * 30);
    drawAntiAliasedHeart(hx, 16, 5, (h % 2 == 0) ? COLOR_ROSE : COLOR_PINK_DEEP);
    drawAntiAliasedHeart(hx, 286, 5, (h % 2 == 0) ? COLOR_PINK_DEEP : COLOR_ROSE);
  }

  // 5. Floating Ambient Sparkles
  int starPositions[][2] = { {34, 48}, {206, 48}, {30, 146}, {210, 146}, {34, 220}, {206, 220} };
  for (int s = 0; s < 6; s++) {
    drawAntiAliasedSparkle(starPositions[s][0], starPositions[s][1], 3, COLOR_GOLD);
  }

  // 6. Top Milestone Pill Badge (Width = 144, Left = 48)
  gfx->fillRoundRect(48, 24, 144, 18, 9, COLOR_CARD_BG);
  gfx->drawRoundRect(48, 24, 144, 18, 9, COLOR_GOLD);
  drawCenteredTextInBox("* CHAPTER 20 *", 48, 144, 29, 1, COLOR_GOLD, COLOR_CARD_BG);

  // 7. Grand Romantic Birthday Titles
  drawSpacedText("HAPPY BIRTHDAY", 48, 2, 2, COLOR_WHITE, 0x4104, COLOR_BG);
  drawSpacedText("RESHMA", 70, 3, 4, COLOR_GOLD, 0x6002, COLOR_BG);

  // 8. Centerpiece: Grand Winged Royal Medallion with Living Pixel Heart (Y = 104 to 208)
  drawAngelWings(120, 154, 52, 0xCE79);

  // Medallion Body (Width = 184, Left = 28, Right = 212, Height = 96)
  gfx->fillRoundRect(28, 106, 184, 96, 12, COLOR_CARD_BG);
  gfx->drawRoundRect(28, 106, 184, 96, 12, COLOR_GOLD);
  gfx->drawRoundRect(30, 108, 180, 92, 10, COLOR_PINK_DEEP);

  // Royal Tiara Crown atop Medallion
  drawCrown(120, 98, COLOR_GOLD);

  // Subtitle inside Medallion
  drawCenteredTextInBox("My Whole Heart * Always", 28, 184, 186, 1, COLOR_ROSE, COLOR_CARD_BG);

  // 9. Romantic Love Banner (Width = 192, Left = 24, Right = 216)
  gfx->fillRoundRect(24, 212, 192, 20, 6, 0x2004);
  gfx->drawRoundRect(24, 212, 192, 20, 6, COLOR_ROSE);
  drawCenteredTextInBox("Forever My Favorite Person <3", 24, 192, 217, 1, COLOR_WHITE, 0x2004);

  // 10. Sweet Footer Ribbon Message
  drawCenteredText("Loved more than all the stars", 240, 1, COLOR_MUTED, COLOR_BG);

  // 11. Living Animated Beating Pixel Heart inside Medallion & Fairy Sparkles
  for (int cycle = 0; cycle < 14; cycle++) {
    int pSize = (cycle % 2 == 0) ? 6 : 5;
    uint16_t core = (cycle % 2 == 0) ? COLOR_HEART_RED : COLOR_ROSE;

    // Cleanly wipe heart zone inside medallion
    gfx->fillRect(80, 114, 80, 68, COLOR_CARD_BG);

    // Draw Living Pixel Art Heart
    drawPixelArtHeart(120, 148, pSize, core, COLOR_GOLD);

    // Floating side hearts
    int floatY1 = 260 + (cycle % 3) * 2;
    int floatY2 = 264 - (cycle % 3) * 2;
    gfx->fillCircle(74, 260, 6, COLOR_BG);
    gfx->fillCircle(74, 264, 6, COLOR_BG);
    gfx->fillCircle(166, 260, 6, COLOR_BG);
    gfx->fillCircle(166, 264, 6, COLOR_BG);
    
    drawAntiAliasedHeart(74, floatY1, 6, COLOR_PINK_DEEP);
    drawAntiAliasedHeart(166, floatY2, 6, COLOR_PINK_DEEP);

    if (cycle % 2 == 0) {
      drawAntiAliasedSparkle(52, 150, 3, COLOR_GOLD);
      drawAntiAliasedSparkle(188, 150, 3, COLOR_GOLD);
      drawAntiAliasedSparkle(120, 262, 3, COLOR_WHITE);
    }
    delay(320);
  }
}

// Draw Vintage Postage Stamp with Heart Motif & Wavy Cancellation
void drawVintageStamp(int x, int y) {
  gfx->fillRect(x, y, 28, 32, 0xF7BE);
  gfx->drawRect(x, y, 28, 32, COLOR_GOLD_DARK);
  gfx->drawRect(x + 2, y + 2, 24, 28, 0xDDB4);

  // Perforation dots
  for (int p = 0; p < 5; p++) {
    gfx->drawPixel(x + 3 + (p * 5), y, 0xCE79);
    gfx->drawPixel(x + 3 + (p * 5), y + 31, 0xCE79);
  }
  for (int p = 0; p < 6; p++) {
    gfx->drawPixel(x, y + 3 + (p * 5), 0xCE79);
    gfx->drawPixel(x + 27, y + 3 + (p * 5), 0xCE79);
  }

  // Stamp Heart Center
  drawAntiAliasedHeart(x + 14, y + 16, 8, COLOR_HEART_RED);
  
  // Stamp denomination
  gfx->setTextSize(1);
  gfx->setTextColor(0x528A, 0xF7BE);
  gfx->setCursor(x + 4, y + 4);
  gfx->print("20c");

  // Wavy Postal Cancellation Lines
  for (int w = 0; w < 3; w++) {
    int wy = y + 8 + (w * 7);
    gfx->drawFastHLine(x - 8, wy, 8, 0x8410);
    gfx->drawFastHLine(x - 6, wy + 1, 6, 0x8410);
  }
}

// Draw 3D Royal Wax Seal Stamp
void drawWaxSeal(int cx, int cy, int size) {
  gfx->fillCircle(cx - 2, cy + 1, size + 2, 0x7800);
  gfx->fillCircle(cx + 2, cy - 1, size + 2, 0x7800);
  gfx->fillCircle(cx, cy, size + 3, 0x9000);

  gfx->fillCircle(cx, cy, size, COLOR_HEART_RED);
  gfx->drawCircle(cx, cy, size - 2, 0x7800);
  gfx->drawCircle(cx, cy, size - 3, COLOR_GOLD);

  drawAntiAliasedHeart(cx, cy, size - 4, COLOR_GOLD, 0x7800);
  gfx->drawPixel(cx - size / 2, cy - size / 2, 0xFFFF);
}

/* ============================================================
 * SCENE 2: Heartfelt Romantic Poetic Love Letter (Full Parchment)
 * ============================================================ */
void renderScene2_LoveNotes() {
  gfx->fillScreen(COLOR_BG);
  drawFooterDots(1);

  // 1. Antique Ivory Dream Parchment (Y = 8 to 296, Height = 288)
  gfx->fillRoundRect(8, 8, 224, 290, 8, 0xFFFA);
  gfx->drawRoundRect(8, 8, 224, 290, 8, COLOR_GOLD);
  gfx->drawRoundRect(10, 10, 220, 286, 6, 0xE718);

  // 2. Vintage Postage Stamp in Top-Right Corner
  drawVintageStamp(194, 14);

  // 3. Delicate Pink Ruled Stationery Lines (8 Beautifully Spaced Lines)
  int lineYPositions[] = { 46, 70, 94, 118, 142, 166, 192, 216 };
  for (int l = 0; l < 8; l++) {
    int ly = lineYPositions[l];
    for (int lx = 18; lx < 222; lx += 4) {
      gfx->drawPixel(lx, ly + 2, 0xEE77);
    }
  }

  // 4. Salutation (Crisp & Heartfelt)
  drawAntiAliasedHeart(22, 24, 7, COLOR_HEART_RED);
  gfx->setTextSize(1);
  gfx->setTextColor(0x3001, 0xFFFA); // Rich Walnut Ink
  gfx->setCursor(34, 20);
  gfx->print("Dearest Reshma,");

  // 5. Soulful, Romantic Love Letter Verses
  const char* stanzas[] = {
    "Happy 20th Birthday, my love.",
    "In a world that spins so fast,",
    "you are my quiet, golden light.",
    "Watching you bloom into this",
    "chapter fills my entire soul",
    "with endless warmth and peace.",
    "Thank you for being my home.",
    "I love you beyond all stars."
  };

  for (int s = 0; s < 8; s++) {
    int curY = lineYPositions[s] - 10;
    uint16_t inkCol = (s == 0) ? 0x9000 : ((s == 7) ? 0x8800 : ((s % 2 == 0) ? 0x2001 : 0x4802));

    gfx->setTextSize(1);
    gfx->setTextColor(inkCol, 0xFFFA);
    gfx->setCursor(20, curY);

    const char* p = stanzas[s];
    while (*p) {
      gfx->print(*p++);
      delay(16);
    }
    delay(90);
  }

  // 6. Sign-off & 3D Royal Wax Seal Stamp
  gfx->setTextColor(0x5140, 0xFFFA);
  gfx->setCursor(20, 238);
  gfx->print("Forever & always yours,");

  // Pulsing Royal Wax Seal at Bottom Center
  for (int pulse = 0; pulse < 10; pulse++) {
    int sealSize = 13 + (pulse % 2);
    gfx->fillCircle(120, 268, 18, 0xFFFA);
    drawWaxSeal(120, 268, sealSize);
    
    if (pulse % 2 == 0) {
      drawAntiAliasedSparkle(86, 266, 3, COLOR_GOLD);
      drawAntiAliasedSparkle(154, 266, 3, COLOR_GOLD);
    }
    delay(380);
  }
}

// Draw Party Festival Bunting along the top
void drawPartyBunting(int y) {
  uint16_t flagColors[] = { COLOR_GOLD, COLOR_ROSE, COLOR_PINK_DEEP, 0x07FF, COLOR_GOLD, COLOR_ROSE, COLOR_PINK_DEEP, 0x07FF, COLOR_GOLD };
  gfx->drawFastHLine(14, y, 212, 0x528A);
  
  for (int b = 0; b < 9; b++) {
    int fx = 20 + (b * 23);
    gfx->fillTriangle(fx, y, fx + 18, y, fx + 9, y + 10, flagColors[b]);
    gfx->drawTriangle(fx, y, fx + 18, y, fx + 9, y + 10, 0x2186);
    gfx->fillCircle(fx + 9, y, 1, COLOR_WHITE);
  }
}

// Draw cute party balloon with string
void drawBalloon(int cx, int cy, int rx, int ry, uint16_t color) {
  gfx->fillCircle(cx, cy, rx, color);
  gfx->fillTriangle(cx - 2, cy + ry - 2, cx + 2, cy + ry - 2, cx, cy + ry + 2, color);
  gfx->drawPixel(cx - rx / 2, cy - ry / 2, COLOR_WHITE);
  gfx->drawPixel(cx - rx / 2 + 1, cy - ry / 2, COLOR_WHITE);
  gfx->drawFastVLine(cx, cy + ry + 2, 8, 0x8410);
  gfx->drawFastHLine(cx - 2, cy + ry + 10, 4, 0x8410);
  gfx->drawFastVLine(cx + 2, cy + ry + 10, 8, 0x8410);
}

/* ============================================================
 * SCENE 3: Completely Reworked Grand 3D Birthday Cake Celebration
 * ============================================================ */
void renderScene3_Cake() {
  gfx->fillScreen(COLOR_BG);
  drawFooterDots(2);

  // 1. Party Festival Bunting Sky
  drawPartyBunting(12);

  // 2. Cute Floating Balloon Bouquets
  drawBalloon(22, 46, 9, 11, COLOR_ROSE);
  drawBalloon(36, 36, 8, 10, COLOR_GOLD);
  drawBalloon(218, 46, 9, 11, COLOR_ROSE);
  drawBalloon(204, 36, 8, 10, COLOR_GOLD);

  // 3. Header Banner (Width = 196, Left = 22)
  drawSpacedText("HAPPY BIRTHDAY", 26, 2, 2, COLOR_GOLD, 0x3186, COLOR_BG);
  drawCenteredText("Make A Wish, Reshma! <3", 46, 1, COLOR_ROSE, COLOR_BG);

  // 4. Cake Pedestal Stand (Width = 184, Left = 28, Right = 212, Center = 120)
  gfx->fillRoundRect(28, 204, 184, 8, 4, 0xD6BA); // Silver Plate
  gfx->drawRoundRect(28, 204, 184, 8, 4, COLOR_GOLD);
  gfx->fillRect(96, 212, 48, 8, 0x94B2); // Pedestal Neck
  gfx->fillRect(88, 220, 64, 4, 0x632C); // Base Rim

  // 5. Tier 1: Base Tier (Width = 164, Left = 38, Right = 202, Y = 156 to 204)
  gfx->fillRoundRect(38, 156, 164, 48, 6, COLOR_CAKE_SPONGE);
  gfx->drawRoundRect(38, 156, 164, 48, 6, COLOR_GOLD);
  gfx->fillRoundRect(38, 156, 164, 14, 4, COLOR_CAKE_FROST); // Ganache Drip

  // Sugar Pearls & Strawberries on Base Tier
  for (int sc = 0; sc < 6; sc++) {
    int sx = 38 + 14 + (sc * 27);
    gfx->fillCircle(sx, 168, 5, COLOR_CAKE_FROST);
    gfx->fillCircle(sx, 168, 2, COLOR_WHITE);
    drawAntiAliasedHeart(sx, 194, 4, COLOR_HEART_RED);
  }

  // Golden Name Badge on Cake
  gfx->fillRoundRect(70, 174, 100, 16, 4, COLOR_CARD_BG);
  gfx->drawRoundRect(70, 174, 100, 16, 4, COLOR_GOLD);
  drawCenteredTextInBox("RESHMA", 70, 100, 178, 1, COLOR_GOLD, COLOR_CARD_BG);

  // 6. Tier 2: Top Tier (Width = 112, Left = 64, Right = 176, Y = 118 to 156)
  gfx->fillRoundRect(64, 118, 112, 38, 5, COLOR_CAKE_SPONGE);
  gfx->drawRoundRect(64, 118, 112, 38, 5, COLOR_GOLD);
  gfx->fillRoundRect(64, 118, 112, 10, 3, COLOR_CAKE_FROST);

  // Cherries on Top Tier Rim
  drawAntiAliasedHeart(76, 116, 5, COLOR_HEART_RED);
  drawAntiAliasedHeart(120, 116, 5, COLOR_HEART_RED);
  drawAntiAliasedHeart(164, 116, 5, COLOR_HEART_RED);

  // 7. Numeral 20 Birthday Candles (Candle 2 at x=92, Candle 0 at x=148)
  gfx->fillRoundRect(80, 84, 24, 34, 4, COLOR_GOLD);
  gfx->drawFastHLine(82, 92, 20, COLOR_ROSE);
  gfx->drawFastHLine(82, 104, 20, COLOR_ROSE);
  gfx->setTextColor(COLOR_BG, COLOR_GOLD);
  gfx->setTextSize(2);
  gfx->setCursor(86, 94);
  gfx->print("2");

  gfx->fillRoundRect(136, 84, 24, 34, 4, COLOR_GOLD);
  gfx->drawFastHLine(138, 92, 20, COLOR_ROSE);
  gfx->drawFastHLine(138, 104, 20, COLOR_ROSE);
  gfx->setTextColor(COLOR_BG, COLOR_GOLD);
  gfx->setTextSize(2);
  gfx->setCursor(142, 94);
  gfx->print("0");

  // Candle Wicks
  gfx->drawFastVLine(92, 76, 8, 0x0000);
  gfx->drawFastVLine(148, 76, 8, 0x0000);

  // Mini Ribbon Bows at Candle Bases
  drawRibbonBow(92, 116, COLOR_ROSE);
  drawRibbonBow(148, 116, COLOR_ROSE);

  // 8. Dynamic Dancing Dual-Flame Physics (Tri-Tone Gradient Aura)
  for (int f = 0; f < 38; f++) {
    int sway1 = (f % 3) - 1;
    int sway2 = ((f + 1) % 3) - 1;
    int fSize1 = 6 + (f % 2);
    int fSize2 = 6 + ((f + 1) % 2);

    gfx->fillRect(78, 54, 28, 22, COLOR_BG);
    gfx->fillRect(134, 54, 28, 22, COLOR_BG);

    gfx->drawFastVLine(92, 76, 8, 0x0000);
    gfx->drawFastVLine(148, 76, 8, 0x0000);

    // Flame 1
    gfx->fillCircle(92 + sway1, 70, fSize1 + 1, COLOR_FLAME_AURA);
    gfx->fillCircle(92 + sway1, 70, fSize1, COLOR_FLAME_MID);
    gfx->fillTriangle(92 + sway1 - fSize1, 70, 92 + sway1 + fSize1, 70, 92 + sway1, 70 - fSize1 - 5, COLOR_FLAME_MID);
    gfx->fillCircle(92 + sway1, 71, fSize1 - 2, COLOR_FLAME_CORE);

    // Flame 2
    gfx->fillCircle(148 + sway2, 70, fSize2 + 1, COLOR_FLAME_AURA);
    gfx->fillCircle(148 + sway2, 70, fSize2, COLOR_FLAME_MID);
    gfx->fillTriangle(148 + sway2 - fSize2, 70, 148 + sway2 + fSize2, 70, 148 + sway2, 70 - fSize2 - 5, COLOR_FLAME_MID);
    gfx->fillCircle(148 + sway2, 71, fSize2 - 2, COLOR_FLAME_CORE);

    delay(85);
  }

  // 9. Extinguish Flames -> Rising Smoke Wisps (Layer-Preserving)
  gfx->fillRect(78, 54, 28, 22, COLOR_BG);
  gfx->fillRect(134, 54, 28, 22, COLOR_BG);
  gfx->drawFastVLine(92, 76, 8, 0x0000);
  gfx->drawFastVLine(148, 76, 8, 0x0000);

  int prevSmokeX1 = 92, prevSmokeY1 = 72, prevSmokeR1 = 2;
  int prevSmokeX2 = 148, prevSmokeY2 = 72, prevSmokeR2 = 2;

  for (int s = 0; s < 26; s++) {
    int ySmoke = 72 - (s * 3);
    
    // 1. Erase only the previous smoke circle outlines
    if (s > 0) {
      gfx->drawCircle(prevSmokeX1, prevSmokeY1, prevSmokeR1, COLOR_BG);
      gfx->drawCircle(prevSmokeX2, prevSmokeY2, prevSmokeR2, COLOR_BG);
    }

    if (ySmoke > 16) {
      int r = 2 + (s / 5);
      int sx1 = 92 + ((s * 3) % 7) - 3;
      int sx2 = 148 - ((s * 3) % 7) + 3;

      // 2. Draw new smoke rings
      gfx->drawCircle(sx1, ySmoke, r, COLOR_SMOKE);
      gfx->drawCircle(sx2, ySmoke, r, COLOR_SMOKE);

      prevSmokeX1 = sx1; prevSmokeY1 = ySmoke; prevSmokeR1 = r;
      prevSmokeX2 = sx2; prevSmokeY2 = ySmoke; prevSmokeR2 = r;
    }

    // 3. Immediately redraw header text & wicks on every frame so text never gets erased
    drawSpacedText("HAPPY BIRTHDAY", 26, 2, 2, COLOR_GOLD, 0x3186, COLOR_BG);
    drawCenteredText("Make A Wish, Reshma! <3", 46, 1, COLOR_ROSE, COLOR_BG);
    gfx->drawFastVLine(92, 76, 8, 0x0000);
    gfx->drawFastVLine(148, 76, 8, 0x0000);

    delay(35);
  }

  // Erase last smoke ring
  gfx->drawCircle(prevSmokeX1, prevSmokeY1, prevSmokeR1, COLOR_BG);
  gfx->drawCircle(prevSmokeX2, prevSmokeY2, prevSmokeR2, COLOR_BG);
  drawPartyBunting(12);
  drawSpacedText("HAPPY BIRTHDAY", 26, 2, 2, COLOR_GOLD, 0x3186, COLOR_BG);
  drawCenteredText("Make A Wish, Reshma! <3", 46, 1, COLOR_ROSE, COLOR_BG);

  // 10. Grand Celebration Banner (Width = 196, Left = 22, Right = 218)
  gfx->fillRoundRect(22, 234, 196, 24, 6, COLOR_CARD_BG);
  gfx->drawRoundRect(22, 234, 196, 24, 6, COLOR_GOLD);
  drawCenteredTextInBox("* WISH SEALED WITH LOVE *", 22, 196, 241, 1, COLOR_GOLD, COLOR_CARD_BG);
  drawCenteredText("May this chapter bring endless joy", 264, 1, COLOR_ROSE, COLOR_BG);

  // 11. Grand Celebration Fireworks & Floating Confetti
  for (int c = 0; c < 120; c++) {
    int rx = (c % 2 == 0) ? random(10, 65) : random(175, 230);
    int ry = random(20, 290);
    uint16_t colors[] = { COLOR_GOLD, COLOR_ROSE, COLOR_WHITE, COLOR_HEART_RED, COLOR_PINK_DEEP, 0x07FF, 0x7E0 };
    uint16_t col = colors[random(7)];
    
    if (c % 3 == 0) {
      drawAntiAliasedHeart(rx, ry, 5, col);
    } else {
      drawAntiAliasedSparkle(rx, ry, random(2, 4), col);
    }
    if (c % 6 == 0) delay(25);
  }
}

/* ============================================================
 * SCENE 4: The Royal Golden Portal QR Vault (Instant Camera Scan)
 * ============================================================ */
static void drawQRCodeCallback(esp_qrcode_handle_t qrcode) {
  int size = esp_qrcode_get_size(qrcode);
  // Large high-contrast modules for effortless camera capture
  int scale = 5; // 29 * 5 = 145px (or 25 * 6 = 150px)
  if (size <= 25) scale = 6;
  int qrPx = size * scale;
  int xOffset = (240 - qrPx) / 2; // Perfectly centered
  int yOffset = 52 + (176 - qrPx) / 2; // Perfectly centered inside white card

  // Render Crisp, High-Contrast Black on Pure White (Instant Optical Recognition)
  for (int y = 0; y < size; y++) {
    for (int x = 0; x < size; x++) {
      uint16_t col = esp_qrcode_get_module(qrcode, x, y) ? 0x0000 : 0xFFFF;
      gfx->fillRect(xOffset + (x * scale), yOffset + (y * scale), scale, scale, col);
    }
  }
}

void renderScene4_QR() {
  gfx->fillScreen(COLOR_BG);
  drawFooterDots(3);

  // 1. Ornate Gold & Sakura Double Frame (Y = 8 to 296, Height = 288)
  gfx->drawRoundRect(8, 8, 224, 290, 10, COLOR_GOLD);
  gfx->drawRoundRect(10, 10, 220, 286, 8, COLOR_PINK_DEEP);

  // 2. Cute Ribbon Bows at Top & Bottom Centers
  drawRibbonBow(120, 8, COLOR_ROSE);
  drawRibbonBow(120, 296, COLOR_ROSE);

  // 3. Cute 4-Corner Heart Clusters
  drawHeartCluster(24, 24);
  drawHeartCluster(216, 24);
  drawHeartCluster(24, 280);
  drawHeartCluster(216, 280);

  // 4. Scalloped Pastel Heart Garland along Top & Bottom
  for (int h = 0; h < 5; h++) {
    int hx = 60 + (h * 30);
    drawAntiAliasedHeart(hx, 16, 5, (h % 2 == 0) ? COLOR_ROSE : COLOR_PINK_DEEP);
    drawAntiAliasedHeart(hx, 286, 5, (h % 2 == 0) ? COLOR_PINK_DEEP : COLOR_ROSE);
  }

  // 5. Crown & Minimal Single Header Pill (Width = 156, Left = 42, Y = 22)
  drawCrown(120, 14, COLOR_GOLD);
  gfx->fillRoundRect(42, 24, 156, 18, 9, COLOR_CARD_BG);
  gfx->drawRoundRect(42, 24, 156, 18, 9, COLOR_GOLD);
  drawCenteredTextInBox("* SCAN TO UNLOCK *", 42, 156, 29, 1, COLOR_GOLD, COLOR_CARD_BG);

  // 6. High-Contrast Crystalline QR Gift Vault (Width = 180, Height = 176, Center = 120, Y = 52 to 228)
  // Generous 17px white quiet-zone around all 4 borders for instant optical trigger
  gfx->fillRoundRect(30, 52, 180, 176, 12, COLOR_WHITE);
  gfx->drawRoundRect(28, 50, 184, 180, 14, COLOR_GOLD);
  gfx->drawRoundRect(30, 52, 180, 176, 12, COLOR_ROSE);

  // 4 Symmetrical Corner Ruby Heart Gem Badges on Outer Card Frame
  drawAntiAliasedHeart(38, 60, 6, COLOR_HEART_RED);
  drawAntiAliasedHeart(202, 60, 6, COLOR_HEART_RED);
  drawAntiAliasedHeart(38, 220, 6, COLOR_HEART_RED);
  drawAntiAliasedHeart(202, 220, 6, COLOR_HEART_RED);

  // 7. Generate High-Contrast Standard QR Code via ESP32 Hardware Engine (Instant Scanning)
  esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
  cfg.display_func = drawQRCodeCallback;
  cfg.max_qrcode_version = 10;
  cfg.qrcode_ecc_level = ESP_QRCODE_ECC_LOW; // Fewer modules = Larger pixels = Instant optical scan!
  esp_qrcode_generate(&cfg, WEBSITE_URL);

  // 8. Symmetrical Laser Radar Scan Bar (Y = 244)
  for (int p = 0; p < 24; p++) {
    int barX = 36 + ((p * 8) % 136);
    gfx->fillRoundRect(36, 244, 168, 4, 2, COLOR_CARD_BG);
    gfx->fillRoundRect(barX, 244, 32, 4, 2, COLOR_GOLD);

    // Decorative baby hearts & sparkles beneath laser bar
    if (p == 0) {
      drawAntiAliasedHeart(68, 268, 6, COLOR_PINK_DEEP);
      drawAntiAliasedHeart(120, 268, 8, COLOR_HEART_RED, COLOR_GOLD);
      drawAntiAliasedHeart(172, 268, 6, COLOR_PINK_DEEP);
      drawAntiAliasedSparkle(94, 268, 2, COLOR_GOLD);
      drawAntiAliasedSparkle(146, 268, 2, COLOR_GOLD);
    }

    delay(380);
  }
}

/* ============================================================
 * Arduino Setup & Main Lifecycle
 * ============================================================ */
void setup() {
  Serial.begin(115200);

  // Enable LCD Backlight
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);

  // 80 MHz Hardware SPI Bus initialization
  bus->begin(80000000UL);
  gfx->begin(80000000UL);

  // Haute-Horlogerie Luxury Boot Sequence
  runBootAnimation();
}

void loop() {
  // 1. Royal Birthday Welcome & Heartbeat (Scene 1)
  renderScene1_Welcome();
  delay(1000);

  // 2. Dreamlike Handwritten Love Letter on Parchment (Scene 2)
  renderScene2_LoveNotes();
  delay(1000);

  // 3. 3-Tier Birthday Cake & Candle Smoke Physics (Scene 3)
  renderScene3_Cake();
  delay(1200);

  // 4. Scannable Giant Golden Portal QR Key & Radar (Scene 4)
  renderScene4_QR();
  delay(800);
}
