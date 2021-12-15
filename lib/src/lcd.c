#include <stdbool.h>
#include <assert.h>

#include <delay.h>
#include <fonts.h>
#include <gpio.h>
#include <lcd_board_def.h>

#include "lcd.h" // quotation marks include the modified header

/** 
  * The more advanced LCD driver (not only text mode) for 
  * ST7735S controller and STM32F2xx or STM32F4xx.
  *
  * Adapted from the basic LCD driver provided by Marcin Peczarski 
  * and Marcin Engel.
  */

/* 
 * Microcontroller pin definitions:
 * constants LCD_*_GPIO_N are port letter codes (A, B, C, ...),
 * constants LCD_*_PIN_N are the port output numbers (from 0 to 15),
 * constants GPIO_LCD_* are memory pointers,
 * constants PIN_LCD_* and RCC_LCD_* are bit masks. 
 */

#define GPIO_LCD_CS   xcat(GPIO, LCD_CS_GPIO_N)
#define GPIO_LCD_A0   xcat(GPIO, LCD_A0_GPIO_N)
#define GPIO_LCD_SDA  xcat(GPIO, LCD_SDA_GPIO_N)
#define GPIO_LCD_SCK  xcat(GPIO, LCD_SCK_GPIO_N)

#define PIN_LCD_CS    (1U << LCD_CS_PIN_N)
#define PIN_LCD_A0    (1U << LCD_A0_PIN_N)
#define PIN_LCD_SDA   (1U << LCD_SDA_PIN_N)
#define PIN_LCD_SCK   (1U << LCD_SCK_PIN_N)

#define RCC_LCD_CS    xcat3(RCC_AHB1ENR_GPIO, LCD_CS_GPIO_N, EN)
#define RCC_LCD_A0    xcat3(RCC_AHB1ENR_GPIO, LCD_A0_GPIO_N, EN)
#define RCC_LCD_SDA   xcat3(RCC_AHB1ENR_GPIO, LCD_SDA_GPIO_N, EN)
#define RCC_LCD_SCK   xcat3(RCC_AHB1ENR_GPIO, LCD_SCK_GPIO_N, EN)

/* Some color definitions */

#define LCD_COLOR_WHITE    0xFFFF
#define LCD_COLOR_BLACK    0x0000
#define LCD_COLOR_GREY     0xF7DE
#define LCD_COLOR_BLUE     0x001F
#define LCD_COLOR_BLUE2    0x051F
#define LCD_COLOR_RED      0xF800
#define LCD_COLOR_MAGENTA  0xF81F
#define LCD_COLOR_GREEN    0x07E0
#define LCD_COLOR_CYAN     0x7FFF
#define LCD_COLOR_YELLOW   0xFFE0

// colors picked in gimp and exported to bmp
#define LCD_BETTER_RED     0xc000
#define LCD_BETTER_BLUE    0x27f
#define LCD_BETTER_GREEN   0xd41
#define LCD_BETTER_YELLOW  0xd680

/* Needed delay(s)  */

#define Tinit   150
#define T120ms  (MAIN_CLOCK_MHZ * 120000 / 4)

/* Text mode globals */

static const font_t *CurrentFont;
static uint16_t TextColor = LCD_COLOR_BLACK;
static uint16_t BackColor = LCD_COLOR_WHITE;

/* Current character line and position, the number of lines, the
number of characters in a line, position 0 and line 0 offset on screen
in pixels */

static int Line, Position, TextHeight, TextWidth, XOffset, YOffset;

/** Internal functions **/

/* The following four functions are inlined and "if" statement is
eliminated during optimization if the "bit" argument is a constant. */

static void CS(uint32_t bit) {
  if (bit) {
    GPIO_LCD_CS->BSRR = PIN_LCD_CS; /* Activate chip select line. */
  }
  else {
    GPIO_LCD_CS->BSRR = PIN_LCD_CS << 16; /* Deactivate chip select line. */
  }
}

static void A0(uint32_t bit) {
  if (bit) {
    GPIO_LCD_A0->BSRR = PIN_LCD_A0; /* Set data/command line to data. */
  }
  else {
    GPIO_LCD_A0->BSRR = PIN_LCD_A0 << 16; /* Set data/command line to command. */
  }
}

static void SDA(uint32_t bit) {
  if (bit) {
    GPIO_LCD_SDA->BSRR = PIN_LCD_SDA; /* Set data bit one. */
  }
  else {
    GPIO_LCD_SDA->BSRR = PIN_LCD_SDA << 16; /* Set data bit zero. */
  }
}

static void SCK(uint32_t bit) {
  if (bit) {
    GPIO_LCD_SCK->BSRR = PIN_LCD_SCK; /* Rising clock edge. */
  }
  else {
    GPIO_LCD_SCK->BSRR = PIN_LCD_SCK << 16; /* Falling clock edge. */
  }
}

static void RCCconfigure(void) {
  /* Enable GPIO clocks. */
  RCC->AHB1ENR |= RCC_LCD_CS | RCC_LCD_A0 | RCC_LCD_SDA | RCC_LCD_SCK;
}

static void GPIOconfigure(void) {
  CS(1); /* Set CS inactive. */
  GPIOoutConfigure(GPIO_LCD_CS, LCD_CS_PIN_N, GPIO_OType_PP,
                   GPIO_High_Speed, GPIO_PuPd_NOPULL);

  A0(1); /* Data are sent default. */
  GPIOoutConfigure(GPIO_LCD_A0, LCD_A0_PIN_N, GPIO_OType_PP,
                   GPIO_High_Speed, GPIO_PuPd_NOPULL);

  SDA(0);
  GPIOoutConfigure(GPIO_LCD_SDA, LCD_SDA_PIN_N, GPIO_OType_PP,
                   GPIO_High_Speed, GPIO_PuPd_NOPULL);

  SCK(0); /* Data bit is written on rising clock edge. */
  GPIOoutConfigure(GPIO_LCD_SCK, LCD_SCK_PIN_N, GPIO_OType_PP,
                   GPIO_High_Speed, GPIO_PuPd_NOPULL);
}

static void LCDwriteSerial(uint32_t data, uint32_t length) {
  uint32_t mask;

  mask = 1U << (length - 1);
  while (length > 0) {
    SDA(data & mask); /* Set bit. */
    --length;         /* Add some delay. */
    SCK(1);           /* Rising edge writes bit. */
    mask >>= 1;       /* Add some delay. */
    SCK(0);           /* Falling edge ends the bit transmission. */
  }
}

static void LCDwriteCommand(uint32_t data) {
  A0(0);
  LCDwriteSerial(data, 8);
  A0(1);
}

static void LCDwriteData8(uint32_t data) {
  /* A0(1); is already set */
  LCDwriteSerial(data, 8);
}

static void LCDwriteData16(uint32_t data) {
  /* A0(1); is already set */
  LCDwriteSerial(data, 16);
}

static void LCDwriteData24(uint32_t data) {
  /* A0(1); is already set */
  LCDwriteSerial(data, 24);
}

static void LCDwriteData32(uint32_t data) {
  /* A0(1); is already set */
  LCDwriteSerial(data, 32);
}

void LCDsetRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  LCDwriteCommand(0x2A);
  LCDwriteData16(x1);
  LCDwriteData16(x2);

  LCDwriteCommand(0x2B);
  LCDwriteData16(y1);
  LCDwriteData16(y2);

  LCDwriteCommand(0x2C);
}

void LCDsetFullRectangle() {
  LCDsetRectangle(0, 0, LCD_PIXEL_WIDTH - 1, LCD_PIXEL_HEIGHT - 1);
}

static void LCDcontrollerConfigure(void) {
  /* Activate chip select */
  CS(0);

  Delay(Tinit);

  /* Sleep out */
  LCDwriteCommand(0x11);

  Delay(T120ms);

  /* Frame rate */
  LCDwriteCommand(0xB1);
  LCDwriteData24(0x053C3C);
  LCDwriteCommand(0xB2);
  LCDwriteData24(0x053C3C);
  LCDwriteCommand(0xB3);
  LCDwriteData24(0x053C3C);
  LCDwriteData24(0x053C3C);

  /* Dot inversion */
  LCDwriteCommand(0xB4);
  LCDwriteData8(0x03);

  /* Power sequence */
  LCDwriteCommand(0xC0);
  LCDwriteData24(0x280804);
  LCDwriteCommand(0xC1);
  LCDwriteData8(0xC0);
  LCDwriteCommand(0xC2);
  LCDwriteData16(0x0D00);
  LCDwriteCommand(0xC3);
  LCDwriteData16(0x8D2A);
  LCDwriteCommand(0xC4);
  LCDwriteData16(0x8DEE);

  /* VCOM */
  LCDwriteCommand(0xC5);
  LCDwriteData8(0x1A);

  /* Memory and color write direction */
  LCDwriteCommand(0x36);
  LCDwriteData8(0xC0);

  /* Color mode 16 bit per pixel */
  LCDwriteCommand(0x3A);
  LCDwriteData8(0x05);

  /* Gamma sequence */
  LCDwriteCommand(0xE0);
  LCDwriteData32(0x0422070A);
  LCDwriteData32(0x2E30252A);
  LCDwriteData32(0x28262E3A);
  LCDwriteData32(0x00010313);
  LCDwriteCommand(0xE1);
  LCDwriteData32(0x0416060D);
  LCDwriteData32(0x2D262327);
  LCDwriteData32(0x27252D3B);
  LCDwriteData32(0x00010413);

  /* Display on */
  LCDwriteCommand(0x29);

  /* Deactivate chip select */
  CS(1);
}

static void LCDsetColors(uint16_t text, uint16_t back) {
  TextColor = text;
  BackColor = back;
}

static void LCDdrawChar(unsigned c) {
  uint16_t const *p;
  uint16_t x, y, w;
  int      i, j;

  CS(0);
  y = YOffset + CurrentFont->height * Line;
  x = XOffset + CurrentFont->width  * Position;
  LCDsetRectangle(x, y, x + CurrentFont->width - 1, y + CurrentFont->height - 1);
  p = &CurrentFont->table[(c - FIRST_CHAR) * CurrentFont->height];
  for (i = 0; i < CurrentFont->height; ++i) {
    for (j = 0, w = p[i]; j < CurrentFont->width; ++j, w >>= 1) {
      LCDwriteData16(w & 1 ? TextColor : BackColor);
    }
  }
  CS(1);
}

/** Public interface implementation **/

void LCDconfigure() {
  /* See Errata, 2.1.6 Delay after an RCC peripheral clock enabling */
  RCCconfigure();
  /* Initialize global variables. */
  LCDsetFont(&LCD_DEFAULT_FONT);
  LCDsetColors(LCD_COLOR_WHITE, LCD_COLOR_BLACK);
  /* Initialize hardware. */
  GPIOconfigure();
  LCDcontrollerConfigure();
  LCDclear();
}

void LCDclear() {
  int i, j;

  CS(0);
  LCDsetFullRectangle();
  for (i = 0; i < LCD_PIXEL_WIDTH; ++i) {
    for (j = 0; j < LCD_PIXEL_HEIGHT; ++j) {
      LCDwriteData16(BackColor);
    }
  }
  CS(1);

  LCDgoto(0, 0);
}

void LCDgoto(int textLine, int charPos) {
  Line = textLine;
  Position = charPos;
}

void LCDputchar(char c) {
  if (c == '\n')
    LCDgoto(Line + 1, 0); /* line feed */
  else if (c == '\r')
    LCDgoto(Line, 0); /* carriage return */
  else if (c == '\t')
    LCDgoto(Line, (Position + 8) & ~7); /* tabulator */
  else {
    if (c >= FIRST_CHAR && c <= LAST_CHAR &&
        Line >= 0 && Line < TextHeight &&
        Position >= 0 && Position < TextWidth) {
      LCDdrawChar(c);
    }
    LCDgoto(Line, Position + 1);
  }
}

void LCDputcharWrap(char c) {
  /* Check if, there is room for the next character,
  but does not wrap on white character. */
  if (Position >= TextWidth &&
      c != '\t' && c != '\r' &&  c != '\n' && c != ' ') {
    LCDputchar('\n');
  }
  LCDputchar(c);
}

void LCDsetFont(const font_t *font) {
  CurrentFont = font;
  TextHeight = LCD_PIXEL_HEIGHT / CurrentFont->height;
  TextWidth  = LCD_PIXEL_WIDTH  / CurrentFont->width;
  XOffset = (LCD_PIXEL_WIDTH  - TextWidth  * CurrentFont->width)  / 2;
  // don't have the space for padding
  YOffset = 0; //(LCD_PIXEL_HEIGHT - TextHeight * CurrentFont->height) / 2;
}

// Advanced interface implementation

#define BSIZE (LCD_PIXEL_WIDTH * LCD_PIXEL_HEIGHT)
static const uint16_t board_pixels[BSIZE] = { 
  #include "board.txt"
};

// Can make space above board for text display
#define BOARD_FIRST_PIXEL 0 //CurrentFont->height

void LCDdrawBoard() {
  CS(0);
  LCDsetFullRectangle();
  for (int i = 0; i < BSIZE; ++i) {
    LCDwriteData16(board_pixels[i]);
  }
  CS(1);
  LCDgoto(0, 0);
}

#define NOTE_WIDTH 32
#define NOTE_HEIGHT 20

#define NOTE_SIZE (NOTE_WIDTH * NOTE_HEIGHT)

// treating this one as extremely poor alpha channel
static const uint16_t note_pixels[NOTE_SIZE] = {
  #include "note.txt"
};

static const uint16_t color_map[4] = {
  LCD_BETTER_BLUE,
  LCD_BETTER_GREEN,
  LCD_BETTER_YELLOW,
  LCD_BETTER_RED
};

#define IMIN(m1, m2) \
  ({ \
    int l = m1, r = m2; \
    l > r ? r : l; \
  })

#define IMAX(m1, m2) \
  ({ \
    int l = m1, r = m2; \
    l < r ? r : l; \
  })


typedef uint16_t (*PixelCalculator)(int x, int y, int px, int py);

#define GET_RED(pixel) ((pixel & LCD_COLOR_RED) >> 11)
#define GET_GREEN(pixel) ((pixel & LCD_COLOR_GREEN) >> 5)
#define GET_BLUE(pixel) (pixel & LCD_COLOR_BLUE)
#define SET_RED(pixel, val) \
  do { \
    static_assert(sizeof(pixel) == sizeof(uint16_t), "setting color of non 16-bit value"); \
    static_assert(sizeof(val) == sizeof(uint16_t), "setting color with non 16-bit value"); \
    pixel &= ~LCD_COLOR_RED; \
    pixel |= val << 11; \
  } while (false)
#define SET_GREEN(pixel, val) \
  do { \
    static_assert(sizeof(pixel) == sizeof(uint16_t), "setting color of non 16-bit value"); \
    static_assert(sizeof(val) == sizeof(uint16_t), "setting color with non 16-bit value"); \
    pixel &= ~LCD_COLOR_GREEN; \
    pixel |= val << 5; \
  } while (false)
#define SET_BLUE(pixel, val) \
  do { \
    static_assert(sizeof(pixel) == sizeof(uint16_t), "setting color of non 16-bit value"); \
    static_assert(sizeof(val) == sizeof(uint16_t), "setting color with non 16-bit value"); \
    pixel &= ~LCD_COLOR_BLUE; \
    pixel |= val; \
  } while (false)
#define SHIFT_RED(val) (val << 11)
#define SHIFT_GREEN(val) (val << 5)
#define SHIFT_BLUE(val) (val)
static_assert(
  ( GET_RED(LCD_COLOR_WHITE) << 11 
  | GET_GREEN(LCD_COLOR_WHITE) << 5 
  | GET_BLUE(LCD_COLOR_WHITE) 
  ) == LCD_COLOR_WHITE, "bad color getters");
static_assert(GET_RED(LCD_COLOR_WHITE) << 11 == LCD_COLOR_RED, "bad red getter");
static_assert(GET_GREEN(LCD_COLOR_WHITE) << 5 == LCD_COLOR_GREEN, "bad green getter");
static_assert(GET_BLUE(LCD_COLOR_WHITE) == LCD_COLOR_BLUE, "bad blue getter");

static_assert(
  ( SHIFT_RED(GET_RED(LCD_COLOR_WHITE)) 
  | SHIFT_GREEN(GET_GREEN(LCD_COLOR_WHITE)) 
  | SHIFT_BLUE(GET_BLUE(LCD_COLOR_WHITE))
  ) == LCD_COLOR_WHITE, "bad shift");
static_assert(SHIFT_RED(GET_RED(LCD_COLOR_WHITE)) == LCD_COLOR_RED, "bad red shift");
static_assert(SHIFT_GREEN(GET_GREEN(LCD_COLOR_WHITE)) == LCD_COLOR_GREEN, "bad green shift");
static_assert(SHIFT_BLUE(GET_BLUE(LCD_COLOR_WHITE)) == LCD_COLOR_BLUE, "bad blue shift");

#define MAX_RED GET_RED(LCD_COLOR_RED)
#define MAX_GREEN GET_GREEN(LCD_COLOR_GREEN)
#define MAX_BLUE GET_BLUE(LCD_COLOR_BLUE)

uint16_t color_pixel = 0;

uint16_t calculateAlpha(uint16_t bg_pixel, uint16_t img_pixel, uint16_t img_alpha) {
  // make calculations in 32-bit signed ints to minimize precision loss

  int32_t board_r = GET_RED(bg_pixel);
  int32_t board_g = GET_GREEN(bg_pixel);
  int32_t board_b = GET_BLUE(bg_pixel);    

  // normally alpha is the same number for each channel
  // but the it was easier to split it
  // and sacrifice some memory efficiency

  int32_t alpha_r = GET_RED(img_alpha);
  int32_t alpha_g = GET_GREEN(img_alpha);
  int32_t alpha_b = GET_BLUE(img_alpha);

  int32_t color_r = GET_RED(img_pixel);
  int32_t color_g = GET_GREEN(img_pixel);
  int32_t color_b = GET_BLUE(img_pixel);

  uint16_t pixel_r = color_r * alpha_r / MAX_RED   + board_r * (MAX_RED   - alpha_r) / MAX_RED;
  uint16_t pixel_g = color_g * alpha_g / MAX_GREEN + board_g * (MAX_GREEN - alpha_g) / MAX_GREEN;
  uint16_t pixel_b = color_b * alpha_b / MAX_BLUE  + board_b * (MAX_BLUE  - alpha_b) / MAX_BLUE;

  return SHIFT_RED(pixel_r) | SHIFT_GREEN(pixel_g) | SHIFT_BLUE(pixel_b);
}

#define BOARD_INDEX(x, y) ((y) * LCD_PIXEL_WIDTH + (x))

static uint16_t getBoardPixelWhenFretPressed(int board_x, int board_y, int col_offset) {
  uint16_t board_pixel = board_pixels[BOARD_INDEX(board_x, board_y)];

  if (FRET_PRESS_Y <= board_y && board_y < FRET_PRESS_Y + 20) {
    int fret_x = col_offset;
    int fret_y = board_y - FRET_PRESS_Y;

    int fret_index = fret_y * NOTE_WIDTH + fret_x;
    
    uint16_t fret_alpha = note_pixels[fret_index];
    uint16_t fret_pixel = color_pixel;

    return calculateAlpha(board_pixel, fret_pixel, fret_alpha);
  } else {
    return board_pixel;
  }
}

// call this before drawing notes
void LCDsetColorPixel(NoteColor color) {
  color_pixel = color_map[color];
}

PixelCalculator makeDrawer(bool isFretPressed) {
  uint16_t lambda(int x, int y, int px, int py) {
    int index = py * NOTE_WIDTH + px;
    uint16_t alpha = note_pixels[index];

    int board_x = x + px;
    int board_y = y + py;
    int board_index = board_y * LCD_PIXEL_WIDTH + board_x;

    uint16_t board_pixel = board_pixels[board_index];

    return calculateAlpha(board_pixel, color_pixel, alpha);
  } // lambda

  uint16_t lambdaPressed(int x, int y, int px, int py) {
    int index = py * NOTE_WIDTH + px;
    uint16_t alpha = note_pixels[index];

    int board_x = x + px;
    int board_y = y + py;

    uint16_t bg_pixel = getBoardPixelWhenFretPressed(board_x, board_y, px);

    return calculateAlpha(bg_pixel, color_pixel, alpha);
  } // lambdaPressed

  return isFretPressed ? lambdaPressed : lambda;
}

uint16_t noteRemover(int x, int y, int px, int py) {
  int board_x = x + px;
  int board_y = y + py;
  int board_index = board_y * LCD_PIXEL_WIDTH + board_x;
  return board_pixels[board_index];
}

uint16_t noteRemoverFretPressed(int x, int y, int px, int py) {
  // TODO
  int board_x = x + px;
  int board_y = y + py;
  int board_index = board_y * LCD_PIXEL_WIDTH + board_x;
  return board_pixels[board_index];
}

// Assumes CS(0) and rectangle set
// Draws only the note with upper-left pixel at (x, y)
// Handles top/bottom edges of the screen correctly, but not left/right
static void drawNoteHelper(int x, int y, PixelCalculator calc) {
  for (
    int py = y < BOARD_FIRST_PIXEL ? BOARD_FIRST_PIXEL - y : 0; // don't start above first pixel
    py < NOTE_HEIGHT &&
      py + y < LCD_PIXEL_HEIGHT; // don't go below last pixel
    ++py
  ) {
    for (int px = 0; px < NOTE_WIDTH; ++px) {
      uint16_t pixel = calc(x, y, px, py);
      LCDwriteData16(pixel);
    }
  }
}

static const int col_x[5] = {-1, 0, 33, 65, 95};

static const NoteColor col_color[5] = {-1, N_RED, N_YELLOW, N_GREEN, N_BLUE};

static void drawBoardLine(int col, int starty, int width, bool isFretPressed) {
  int startx = col_x[col];
  for (int i = 0; i < width; ++i) {
    uint16_t pixel = isFretPressed
      ? getBoardPixelWhenFretPressed(startx + i, starty, i) 
      : board_pixels[BOARD_INDEX(startx + i, starty)];
    
    LCDwriteData16(pixel);
  }
}


void LCDdrawNote(int col, int y) {
  int x = col_x[col];
  NoteColor color = col_color[col];
  LCDsetColorPixel(color);
  CS(0);
  LCDsetRectangle(x, y, x + NOTE_WIDTH - 1, y + NOTE_HEIGHT - 1);
  drawNoteHelper(x, y, makeDrawer(LCDisFretPressed(col)));
  CS(1);
  LCDgoto(0, 0);
}


// This function assumes that a note is displayed in column col at y=oldy
// It draws another note 1 pixel higher/lower according to passed flag
// Also it fills the space unoccupied by the new note with background pixels
// Doesn't draw pixels outside the screen.
void LCDmoveNoteVertical(int col, int oldy, bool up) {
  int x = col_x[col];
  
  NoteColor color = col_color[col];
  LCDsetColorPixel(color);

  bool down = !up;
  bool fret_pressed = LCDisFretPressed(col);

  int upper_bound = IMAX(oldy - up, BOARD_FIRST_PIXEL);
  int lower_bound = IMIN(oldy + down + NOTE_HEIGHT - 1, LCD_PIXEL_HEIGHT - 1);

  if (upper_bound >= LCD_PIXEL_HEIGHT || lower_bound < BOARD_FIRST_PIXEL) {
    // nothing to draw
    return;
  }
  
  CS(0);
  LCDsetRectangle(
    x,                  upper_bound,
    x + NOTE_WIDTH - 1, lower_bound
  );

  // if moving down, overwrite old first row  
  if (down && oldy >= BOARD_FIRST_PIXEL) {
    drawBoardLine(col, oldy, NOTE_WIDTH, fret_pressed);
  }
  
  drawNoteHelper(x, oldy - up + down, makeDrawer(fret_pressed));

  // if moving up, overwrite old last row
  if (up) {
    int lower_line_y = oldy + NOTE_HEIGHT - 1;
    if (lower_line_y < LCD_PIXEL_HEIGHT) {
      drawBoardLine(col, lower_line_y, NOTE_WIDTH, fret_pressed);
    }
  }

  CS(1);
  LCDgoto(0, 0);
}

// removes a note by filling its space with board pixels
void LCDremoveNote(int col, int y) {
  int x = col_x[col];
  int upper_bound = IMAX(y, BOARD_FIRST_PIXEL);
  int lower_bound = IMIN(y + NOTE_HEIGHT - 1, LCD_PIXEL_HEIGHT - 1);
  if (upper_bound >= LCD_PIXEL_HEIGHT || lower_bound < BOARD_FIRST_PIXEL) {
    // nothing to draw
    return;
  }
  CS(0);
  LCDsetRectangle(x, upper_bound, x + NOTE_WIDTH - 1, lower_bound);
  drawNoteHelper(x, y, noteRemover);
  CS(1);
  LCDgoto(0, 0);
}

bool col_pressed[5] = {};

void LCDpressFret(int col) {
  col_pressed[col] = true;
  LCDdrawNote(col, FRET_PRESS_Y);
}

void LCDreleaseFret(int col) {
  LCDremoveNote(col, FRET_PRESS_Y);
  col_pressed[col] = false;
}

bool LCDisFretPressed(int col) {
  return col_pressed[col];
}