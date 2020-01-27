#ifndef SCREEN_H
#define SCREEN_H

#define VGA_ADDRESS 0xB8000

#define BLACK 0x0
#define BLUE 0x1
#define GREEN 0x2
#define CYAN 0x3
#define RED 0x4
#define PURPLE 0x5
#define BROWN 0x6
#define GRAY 0x7
#define DARKGRAY 0x8
#define LIGHTBLUE 0x9
#define LIGHTGREEN 0xA
#define LIGHTCYAN 0xB
#define LIGHTRED 0xC
#define LIGHTPURPLE 0xD
#define YELLOW 0xE
#define WHITE 0xF

#define GREEN_ON_BLACK 0x02
#define WHITE_ON_RED 0x4F
#define DEFAULT_ATTR 0x0F

#define VGA_COLUMNS 80
#define VGA_ROWS 25

extern uint8_t textColor;

void clearCharAt();
void clearScreen();
int getCursorOffset();
int getOffset(int row, int col);
int getOffsetCol(int offset);
int getOffsetRow(int offset);
int printCharAt(int row, int col, char c, char attr);
int printChar(char c, char attr);
void setCursorPos(int row, int col);
void setCursorOffset(int offset);
void clearRow(int row);
void setTextColor(uint8_t fgColor);
void setBackgroundColor(uint8_t bgColor);
void resetScreenColors();
int deleteLastChar();
void setColor(uint8_t);
void kprintf(const char *format, ...);
void kprintfColor(uint8_t color, const char *format, ...);
void activateCursor();
void kPrintOKMessage(const char *message);
void kPrintKOMessage(const char *message);

#endif