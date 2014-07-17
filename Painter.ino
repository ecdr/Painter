/*
 * Painter - IR Remote Controlled Painting Application for the SmartMatrix
 *
 * Copyright (c) 2014 Craig A. Lindley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Version: 1.0
 * Last Update: 07/11/2014
 */

// Hardware chip selects
// If your hardware isn't connected this way, this sketch won't work correctly
#define SD_CARD_CS     15
#define IR_RECV_CS     18

// Include all include files
#include "IRremote.h"
#include "SdFat.h"
#include "SdFatUtil.h"
#include "SmartMatrix_32x32.h"

// Paint file directory
#define PAINT_DIR    "/"

// IR Raw Key Codes for SparkFun remote
#define IRCODE_HOME  0x10EFD827    
#define IRCODE_A     0x10EFF807
#define IRCODE_B     0x10EF7887
#define IRCODE_C     0x10EF58A7
#define IRCODE_UP    0x10EFA05F
#define IRCODE_LEFT  0x10EF10EF
#define IRCODE_SEL   0x10EF20DF
#define IRCODE_RIGHT 0x10EF807F
#define IRCODE_DOWN  0x10EF00FF

// These color defines valid after palette is generated
#define BLACK palette[0]
#define WHITE palette[1]

// Image drawing functions defined in ImageGraphics.cpp
extern void drawPixelIntoImage(int x, int y, byte colorIndex);
extern void drawLineIntoImage(int x1, int y1, int x2, int y2, byte colorIndex);
extern void drawCircleIntoImage(int x0, int y0, int radius, byte colorIndex);
extern void fillCircleIntoImage(int x0, int y0, int radius, byte colorIndex);
extern void drawRectIntoImage(int x0, int y0, int x1, int y1, byte colorIndex);
extern void fillRectIntoImage(int x0, int y0, int x1, int y1, byte colorIndex);
extern void drawTriangleIntoImage(int x1, int y1, int x2, int y2, int x3, int y3, byte colorIndex);
extern void fillTriangleIntoImage(int x1, int y1, int x2, int y2, int x3, int y3, byte colorIndex);

// Create required instances of hardware components
SmartMatrix matrix;                // The 32x32 LED matrix
IRrecv      irReceiver(IR_RECV_CS);// The IR receiver
SdFat       sd;                    // SD card interface

const int DEFAULT_BRIGHTNESS = 100;

const int WIDTH = 32;
const int MINX  = 0;
const int MAXX  = WIDTH - 1;
const int MIDX  = WIDTH / 2;

const int HEIGHT = 32;
const int MINY   = 0;
const int MAXY   = HEIGHT - 1;
const int MIDY   = HEIGHT / 2;

// Memory backing drawn image
byte image[WIDTH][HEIGHT];

// Palette of available colors
// Total number of colors in our palette
const int COLORS = 36;
rgb24 palette[COLORS];

// Current position of drawing cursor
int cursorX;
int cursorY;

// Current position of palette cursor
int paletteCursorX;
int paletteCursorY;

// Current position of tool cursor
int toolCursorX;
int toolCursorY;

// Current working variables
int currentToolIndex;
int toolParameterCount;
int currentToolParameterCount;
byte currentDrawingColorIndex;
byte currentToolColorIndex;

/*******************************************************************/
/***                        IR Functions                         ***/
/*******************************************************************/

// Low level IR code reading function
// Function will return 0 if no IR code available
unsigned long _readIRCode() {
    decode_results results;
    results.value = 0;

    // Attempt to read an IR code ?
    if (irReceiver.decode(&results)) {
        delay(20);
        // Prepare to receive the next IR code
        irReceiver.resume();
    }
    return results.value;
}

// Read an IR code
// Function will return 0 if no IR code available
unsigned long readIRCode() {
    // Is there an IR code to read ?
    unsigned long code = _readIRCode();
    if (code == 0) {
        // No code so return 0
        return 0;
    }
    // Keep reading until code changes
    while (_readIRCode() == code) {
        ;
    }
    return code;
}

unsigned long waitForIRCode() {
    unsigned long irCode = readIRCode();
    while ((irCode == 0) || (irCode == 0xFFFFFFFF)) {
        delay(200);
        irCode = readIRCode();
    }
    return irCode;
}

/*******************************************************************/
/***                Color and Palette Functions                  ***/
/*******************************************************************/

// HSV to RGB color conversion
// Input arguments
// hue in degrees (0 - 360.0)
// saturation (0.0 - 1.0)
// value (0.0 - 1.0)
// Output arguments
// red, green blue (0.0 - 1.0)
void hsvToRGB(float hue, float saturation, float value, float * red, float * green, float * blue) {

    int i;
    float f, p, q, t;

    if (saturation == 0) {
        // achromatic (grey)
        *red = *green = *blue = value;
        return;
    }
    hue /= 60;                  // sector 0 to 5
    i = floor(hue);
    f = hue - i;                // factorial part of h
    p = value * (1 - saturation);
    q = value * (1 - saturation * f);
    t = value * (1 - saturation * (1 - f));
    switch (i) {
    case 0:
        *red = value;
        *green = t;
        *blue = p;
        break;
    case 1:
        *red = q;
        *green = value;
        *blue = p;
        break;
    case 2:
        *red = p;
        *green = value;
        *blue = t;
        break;
    case 3:
        *red = p;
        *green = q;
        *blue = value;
        break;
    case 4:
        *red = t;
        *green = p;
        *blue = value;
        break;
    default:
        *red = value;
        *green = p;
        *blue = q;
        break;
    }
}

// Create a HSV color
rgb24 createHSVColor(float hue, float saturation,  float value) {

    float r, g, b;
    rgb24 color;

    hsvToRGB(hue, saturation, value, &r, &g, &b);
    color.red   = r * 255;
    color.green = g * 255;
    color.blue =  b * 255;
    return color;
}

// Create an HSV color
rgb24 createHSVColor(int divisions, int index, float saturation, float value) {
    float hueAngle = (360.0 * index) / divisions;
    return createHSVColor(hueAngle, saturation, value);
}

// Create an HSV color
rgb24 createAHSVColor(int divisions, int index, float saturation, float value) {
    index %= divisions;
    return createHSVColor(divisions, index, saturation, value);
}

rgb24 createHSVColorWithDivisions(int divisions, int index) {
    return createAHSVColor(divisions, index, 1.0, 1.0);
}

/*******************************************************************/
/***                         Misc Functions                      ***/
/*******************************************************************/

// Print string and int value
void printStrInt(const char *str, int value) {

    Serial.print(str);
    Serial.print(": ");
    Serial.println(value);
}

// Print string and string
void printStrStr(const char *str1, const char *str2) {

    Serial.print(str1);
    Serial.println(str2);
}

extern bitmap_font *font;

// Clear a portion of the screen for overwriting
void clearString(int16_t x, int16_t y, rgb24 color, const char text[]) {
    int xcnt, ycnt, i = 0, offset = 0;
    char character;

    // limit text to 10 chars, why?
    for (i = 0; i < 10; i++) {
        character = text[offset++];
        if (character == '\0')
            return;

        for (ycnt = 0; ycnt < font->Height; ycnt++) {
            for (xcnt = 0; xcnt < font->Width; xcnt++) {
                matrix.drawPixel(x + xcnt, y + ycnt, color);
            }
        }
        x += font->Width;
    }
}

// Refresh screen from backing image data
void refreshScreen() {

    rgb24 color;

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            color = palette[image[x][y]];
            matrix.drawPixel(x, y, color);
        }
    }
    matrix.swapBuffers();
}

// Prepare new image
void newImage() {

    // Fill image array with index of black or 0
    memset(image, 0, sizeof(image));    

    cursorX = MIDX;
    cursorY = MIDY;   
    paletteCursorX = paletteCursorY = 0;
    toolCursorX = toolCursorY = 0;

    // Set a current color index to white
    currentDrawingColorIndex = 1;   
}

// Generate the palette
// Entry 0 is always black; Entry 1 is always white
// This should only ever be called once
void generatePalette() {
    palette[0]  = {  
        0,   0,   0 };
    palette[1] = {
        255, 255, 255 };

    // We must generate 34 other colors covering spectrum
    for (int i = 0; (i < COLORS - 2); i++) {
        palette[i + 2] = createHSVColorWithDivisions(COLORS - 2, i);
    }
}

#define PALETTE_SWATCH_WIDTH  4
#define PALETTE_SWATCH_HEIGHT 4

// Display the color palette
void displayPalette() {

    int colorIndex = 0;
    int x, x1, y, y1;

    matrix.scrollText("Use arrows to select color", 1);

    // Clear screen
    matrix.fillScreen(BLACK);
    matrix.swapBuffers();

    for (int row = 0; row < 6; row++) {
        y = row + 1 + row * PALETTE_SWATCH_HEIGHT;
        y1 = y + PALETTE_SWATCH_HEIGHT - 1;
        for (int col = 0; col < 6; col++) {
            x = col + 1 + col * PALETTE_SWATCH_WIDTH;
            x1 = x + PALETTE_SWATCH_WIDTH - 1;

            // Draw filled rectangle with white outline
            matrix.fillRectangle(x, y, x1, y1, palette[colorIndex++]);
        }
    }
    matrix.swapBuffers();
}

// Get the index of the color under the palette cursor
int getPaletteIndexAtPaletteCursor() {

    return paletteCursorY * 6 + paletteCursorX;
}

// Restore the palette swatch outline on screen
void restorePaletteSwatchOutlineAtCursor() {

    int x, x1, y, y1;

    // Calculate position of black rect around color at the palette cursor
    y = paletteCursorY + 1 + paletteCursorY * PALETTE_SWATCH_HEIGHT;
    y1 = y + PALETTE_SWATCH_HEIGHT - 1;
    x = paletteCursorX + 1 + paletteCursorX * PALETTE_SWATCH_WIDTH;
    x1 = x + PALETTE_SWATCH_WIDTH - 1;

    // Fill rectangle with palette color
    matrix.fillRectangle(x, y, x1, y1,  palette[getPaletteIndexAtPaletteCursor()]);
    matrix.swapBuffers();
}

// Position the palette cursor on screen
void positionPaletteSelectorAtCursor() {

    int x, x1, y, y1;

    // Calculate position of black rect around color at the palette cursor
    y = paletteCursorY + 1 + paletteCursorY * PALETTE_SWATCH_HEIGHT;
    y1 = y + PALETTE_SWATCH_HEIGHT - 1;
    x = paletteCursorX + 1 + paletteCursorX * PALETTE_SWATCH_WIDTH;
    x1 = x + PALETTE_SWATCH_WIDTH - 1;

    // Draw white rectangle around selected palette color
    matrix.fillRectangle(x, y, x1, y1, WHITE);
    matrix.drawRectangle(x + 1, y + 1, x1 - 1, y1 - 1, palette[getPaletteIndexAtPaletteCursor()]);
    matrix.swapBuffers();
}

// Move the palette cursor to the right
void movePaletteCursorRight() {

    restorePaletteSwatchOutlineAtCursor();

    paletteCursorX++;
    if (paletteCursorX >= 6) {
        paletteCursorX = 0;
    }
    positionPaletteSelectorAtCursor();
}

// Move the palette cursor to the left
void movePaletteCursorLeft() {

    restorePaletteSwatchOutlineAtCursor();

    paletteCursorX--;
    if (paletteCursorX < 0) {
        paletteCursorX = 5;
    }
    positionPaletteSelectorAtCursor();
}

// Move the palette cursor up
void movePaletteCursorUp() {

    restorePaletteSwatchOutlineAtCursor();

    paletteCursorY--;
    if (paletteCursorY < 0) {
        paletteCursorY = 5;
    }
    positionPaletteSelectorAtCursor();
}

// Move the palette cursor down
void movePaletteCursorDown() {

    restorePaletteSwatchOutlineAtCursor();

    paletteCursorY++;
    if (paletteCursorY >= 6) {
        paletteCursorY = 0;
    }
    positionPaletteSelectorAtCursor();
}

// Select new color from palette
void selectNewColor() {

    // Display the palette
    displayPalette();
    
    // Draw selection frame around currently selected color
    positionPaletteSelectorAtCursor();

    // Repeat until terminated
    while (true) {
        switch(waitForIRCode()) {
        case IRCODE_SEL:
        case IRCODE_HOME:
            currentDrawingColorIndex = getPaletteIndexAtPaletteCursor();
            return;

        case IRCODE_LEFT:
            movePaletteCursorLeft();
            break;

        case IRCODE_RIGHT:
            movePaletteCursorRight();
            break;

        case IRCODE_UP:
            movePaletteCursorUp();
            break;

        case IRCODE_DOWN:
            movePaletteCursorDown();
            break;
        }
    }
}  

// Position cursor block at cursor
// Pixels in block reflect tool selected
// NOTE: pixels in block cursor only on screen not in imageData
void positionBlockAtCursor() {

    if (cursorY >= 1) {
        matrix.drawPixel(cursorX, cursorY - 1, palette[currentToolColorIndex]);
    }

    if (cursorX >= 1) {
        matrix.drawPixel(cursorX - 1, cursorY, palette[currentToolColorIndex]);
    }

    matrix.drawPixel(cursorX, cursorY, palette[currentDrawingColorIndex]);

    if (cursorX < MAXX) {
        matrix.drawPixel(cursorX + 1, cursorY, palette[currentToolColorIndex]);
    }

    if (cursorY < MAXY) {
        matrix.drawPixel(cursorX, cursorY + 1, palette[currentToolColorIndex]);
    }
    matrix.swapBuffers();
}

// Move the cursor to the right
void moveCursorRight() {

    refreshScreen();

    cursorX++;
    if (cursorX >= WIDTH) {
        cursorX = MINX;
    }
    positionBlockAtCursor();
}

// Move the cursor to the left
void moveCursorLeft() {

    refreshScreen();

    cursorX--;
    if (cursorX < MINX) {
        cursorX = MAXX;
    }
    positionBlockAtCursor();
}

// Move the cursor up
void moveCursorUp() {

    refreshScreen();

    cursorY--;
    if (cursorY < MINY) {
        cursorY = MAXY;
    }
    positionBlockAtCursor();
}

// Move the cursor down
void moveCursorDown() {

    refreshScreen();

    cursorY++;
    if (cursorY >= HEIGHT) {
        cursorY = MINY;
    }
    positionBlockAtCursor();
}

/*******************************************************************/
/***                 Save / Load Image Functions                 ***/
/*******************************************************************/

// Losd an image from a file on the SD card
boolean loadImage(const char *filename) {

    SdFile imageFile;
    char pathname[30];

    // Prepare full pathname
    strcpy(pathname, PAINT_DIR);
    strcat(pathname, filename);

    // Make sure file exists before attempting open
    if (! sd.exists(pathname)) {
        printStrStr((const char *) pathname, " does not exist");
        return false;
    }
    // Attempt to open the specified file for reading
    if (! imageFile.open(pathname, O_READ)) {
        printStrStr("Error opening: ", pathname);
        return false;
    }
    // Attempt to read the image data
    if (imageFile.read(image, sizeof(image)) != sizeof(image)) {
        printStrStr("Error reading: ", pathname);
        imageFile.close();
        return false;
    }
    imageFile.close();

    // Refresh the screen for just loaded image
    refreshScreen();
    return true;
}

// Save or Load an image to/from file
boolean saveImage(const char *filename) {

    SdFile imageFile;
    char pathname[30];

    // Prepare full pathname
    strcpy(pathname, PAINT_DIR);
    strcat(pathname, filename);

    // Attempt to open the specified file for writing
    if (! imageFile.open(pathname, O_WRITE | O_CREAT | O_TRUNC)) {
        printStrStr("Error opening: ", pathname);
        return false;
    }
    // Attempt to write the image data
    if (imageFile.write(image, sizeof(image)) != sizeof(image)) {
        printStrStr("Error writing: ", pathname);
        imageFile.close();
        return false;
    }
    imageFile.close();
    return true;
}

#define MIN_FILE_INDEX 1
#define MAX_FILE_INDEX 9

// User wants to save or load an image
void saveLoadImage() {

    char filename[13];
    int fileIndex = 1;

    rgb24 textColor = {
        128, 128, 128  };
    rgb24 bgColor = {
        0, 0, 0  };

    // Clear screen
    matrix.fillScreen(BLACK);
    matrix.swapBuffers();

    // Format the screen
    matrix.setFont(font3x5);
    matrix.drawString(0,  0, textColor, "SaveLoad");
    matrix.drawString(0,  8, textColor, "File:");
    matrix.drawString(2, 14, textColor, "Save A");
    matrix.drawString(2, 20, textColor, "Load B");
    matrix.drawString(2, 26, textColor, "Exit C");
    matrix.swapBuffers();

    // Repeat until terminated
    while (true) {

        // Form the filename
        strcpy(filename, "F");
        itoa(fileIndex, &filename[1], 10);

        // Clear filename display area on screen
        clearString(20, 8, bgColor, "XX");

        // Display the filename
        matrix.drawString(20, 8, textColor, filename);
        matrix.swapBuffers();

        // Wait for user's input
        switch(waitForIRCode()) {
        case IRCODE_A:
            // User wants to save an image
            saveImage(filename);
            return;

        case IRCODE_B:
            // User wants to load an image
            loadImage(filename);
            return;

        case IRCODE_C:
            // User wants to exit
            return;

        case IRCODE_UP:
        case IRCODE_RIGHT:
            fileIndex++;
            if (fileIndex > MAX_FILE_INDEX) {
                fileIndex = MAX_FILE_INDEX;
            }
            break;

        case IRCODE_DOWN:
        case IRCODE_LEFT:
            fileIndex--;
            if (fileIndex < MIN_FILE_INDEX) {
                fileIndex = MIN_FILE_INDEX;
            }
            break;
        }
    }
}

/*******************************************************************/
/***                   Tool Palette Functions                    ***/
/*******************************************************************/

#define TOOL_ICON_WIDTH  6
#define TOOL_ICON_HEIGHT 6

// Tool types
enum TOOL_TYPE {
    NEW, FILL, PIXEL, LINE, CIRCLE, FILLED_CIRCLE, RECT, FILLED_RECT, TRIANGLE, FILLED_TRIANGLE};

// Tool Icons always 6x6. 0 is black and 1 signifies the tool color.
// Leave perimeter black for selection indication

byte newIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 1, 0, 0, 1, 0,    
    0, 1, 1, 0, 1, 0,    
    0, 1, 0, 1, 1, 0,    
    0, 1, 0, 0, 1, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte fillIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 0,    
    0, 1, 0, 0, 0, 0,    
    0, 1, 1, 1, 0, 0,    
    0, 1, 0, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte pixelIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
    0, 0, 0, 1, 0, 0,    
    0, 0, 0, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte lineIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 0, 0, 0, 1, 0,    
    0, 0, 0, 1, 0, 0,    
    0, 0, 1, 0, 0, 0,    
    0, 1, 0, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte circleIcon[] = {
    0, 0, 0, 0, 0, 0,   
    0, 0, 1, 1, 0, 0,    
    0, 1, 0, 0, 1, 0,    
    0, 1, 0, 0, 1, 0,    
    0, 0, 1, 1, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte filledCircleIcon[] = {
    0, 0, 0, 0, 0, 0,   
    0, 0, 1, 1, 0, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 0, 1, 1, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};


byte rectIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 0,    
    0, 1, 0, 0, 1, 0,    
    0, 1, 0, 0, 1, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte filledRectIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 1, 1, 1, 1, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 1, 1, 1, 1, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte triangleIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 0, 1, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
    0, 0, 0, 0, 0, 0,    
    0, 1, 0, 1, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

byte filledTriangleIcon[] = {
    0, 0, 0, 0, 0, 0,  
    0, 0, 1, 0, 0, 0,    
    0, 0, 1, 0, 0, 0,    
    0, 1, 1, 1, 0, 0,    
    0, 1, 1, 1, 0, 0,    
    0, 0, 0, 0, 0, 0,    
};

typedef struct {
    TOOL_TYPE type;
    int numberOfParameters;
    byte *ptIcon;
    byte toolColorIndex;
} 
TOOL_DATA;

// Define the tools
TOOL_DATA tools[] = {

    {
        NEW,             0, newIcon, 1 }
    , 
    {
        FILL,            0, fillIcon, 1 }
    ,
    {
        PIXEL,           1, pixelIcon, 2 }
    ,
    {
        LINE,            2, lineIcon, 6 }
    ,
    {
        CIRCLE,          2, circleIcon, 10 }
    ,
    {
        FILLED_CIRCLE,   2, filledCircleIcon, 14 }
    ,
    {
        RECT,            2, rectIcon, 18 }
    ,
    {
        FILLED_RECT,     2, filledRectIcon, 22 }
    ,
    {
        TRIANGLE,        3, triangleIcon, 26 }
    ,
    {
        FILLED_TRIANGLE, 3, filledTriangleIcon, 30 }
    ,
};

// Draw the tool icon using tool color from palette
void drawIcon(int xx, int yy, byte *ptIcon, byte toolColorIndex) {

    int colorIndex = 0;

    for (int y = yy; y < yy + TOOL_ICON_HEIGHT; y++) {
        for (int x = xx; x < xx + TOOL_ICON_WIDTH; x++) {
            byte b = ptIcon[colorIndex++];
            if (b == 1) {
                b = toolColorIndex;
            }
            matrix.drawPixel(x, y, palette[b]);
        }
    }
}

// Display the available tools
void displayTools() {

    int x, y, toolIndex = 0;

    matrix.scrollText("Use arrows to select tool", 1);

    for (int row = 0; row < 3; row++) {
        y = 4 + (row * 3) + (row * TOOL_ICON_HEIGHT);
        for (int col = 0; col < 4; col++) {
            x = 2 + col + (col * TOOL_ICON_WIDTH);

            // Get reference to tool
            TOOL_DATA td = tools[toolIndex++];

            // Check if we are out of tools to display
            if (toolIndex == 11) {
                return;
            }

            // Draw tool icon in the appropriate color
            drawIcon(x, y, td.ptIcon, td.toolColorIndex);
        }
    }
    matrix.swapBuffers();
}

// Get the index of the color under the palette cursor
int getToolIndexAtToolCursor() {

    return toolCursorY * 4 + toolCursorX;
}

// Remove the tool cursor on screen
void removeToolSelectorAtCursor() {

    int x, x1, y, y1;

    // Calculate position of black rect around tool icon
    x = 2 + toolCursorX + (toolCursorX * TOOL_ICON_WIDTH);
    x1 = x + TOOL_ICON_WIDTH - 1;
    y = 4 + (toolCursorY * 3) + (toolCursorY * TOOL_ICON_HEIGHT);
    y1 = y + TOOL_ICON_HEIGHT - 1;

    // Draw black frame around selected tool icon
    matrix.drawLine( x, y, x1,  y, BLACK);
    matrix.drawLine(x1, y, x1, y1, BLACK);
    matrix.drawLine(x, y1, x1, y1, BLACK);
    matrix.drawLine(x, y,   x, y1, BLACK);
    matrix.swapBuffers();
}

// Position the tool cursor on screen
void positionToolSelectorAtCursor() {

    int x, x1, y, y1;

    // Calculate position of white rect around tool icon
    x = 2 + toolCursorX + (toolCursorX * TOOL_ICON_WIDTH);
    x1 = x + TOOL_ICON_WIDTH - 1;
    y = 4 + (toolCursorY * 3) + (toolCursorY * TOOL_ICON_HEIGHT);
    y1 = y + TOOL_ICON_HEIGHT - 1;

    // Draw white frame around selected tool icon
    matrix.drawLine( x, y, x1,  y, WHITE);
    matrix.drawLine(x1, y, x1, y1, WHITE);
    matrix.drawLine(x, y1, x1, y1, WHITE);
    matrix.drawLine(x, y,   x, y1, WHITE);
    matrix.swapBuffers();
}

// Move the tool cursor to the right
void moveToolCursorRight() {

    removeToolSelectorAtCursor();

    toolCursorX++;
    if (toolCursorY < 2) {
        if (toolCursorX >= 4) {
            toolCursorX = 0;
        }
    }    
    else    {
        if (toolCursorX >= 2) {
            toolCursorX = 0;
        }
    }
    positionToolSelectorAtCursor();
}

// Move the tool cursor to the left
void moveToolCursorLeft() {

    removeToolSelectorAtCursor();

    toolCursorX--;
    if (toolCursorX < 0) {
        toolCursorX = 3;
    }
    positionToolSelectorAtCursor();
}

// Move the tool cursor up
void moveToolCursorUp() {

    removeToolSelectorAtCursor();

    toolCursorY--;
    if (toolCursorY < 0) {
        toolCursorY = 2;
    }
    positionToolSelectorAtCursor();
}

// Move the palette cursor down
void moveToolCursorDown() {

    removeToolSelectorAtCursor();

    toolCursorY++;
    if (toolCursorX < 2) {
        if (toolCursorY >= 3) {
            toolCursorY = 0;
        }
    }    
    else    {
        if (toolCursorY >= 2) {
            toolCursorY = 0;
        }
    }
    positionToolSelectorAtCursor();
}

// Select a new tool
void selectNewTool() {

    // Clear screen
    matrix.fillScreen(BLACK);

    // Display the tools
    displayTools();

    // Position tool cursor
    positionToolSelectorAtCursor();

    // Loop until tool is selected
    while (true) {
        switch(waitForIRCode()) {
        case IRCODE_SEL:
            // Get index of selected tool
            currentToolIndex = getToolIndexAtToolCursor();

            if (currentToolIndex == NEW) {
                // Clear image for new drawing
                newImage();

                // Override selection with the pixel drawing tool
                currentToolIndex = 2;
            } 
            else if (currentToolIndex == FILL) {
                // Fill image with current color
                memset(image, currentDrawingColorIndex, sizeof(image));

                // Override selection with the pixel drawing tool
                currentToolIndex = 2;
            }

            // Extract parameters from selected or overriden tool          
            toolParameterCount = tools[currentToolIndex].numberOfParameters;
            currentToolParameterCount = toolParameterCount;
            currentToolColorIndex = tools[currentToolIndex].toolColorIndex;
            return;

        case IRCODE_LEFT:
            moveToolCursorLeft();
            break;

        case IRCODE_RIGHT:
            moveToolCursorRight();
            break;

        case IRCODE_UP:
            moveToolCursorUp();
            break;

        case IRCODE_DOWN:
            moveToolCursorDown();
            break;
        }
    }
}

// Structure for holding x, y coordinates
typedef struct {
    int x;
    int y;
} 
POINT;

POINT parameters[2];

/*******************************************************************/
/***                     Main Program Setup                     ***/
/*******************************************************************/

void setup() {

    // Setup serial interface
    Serial.begin(115200);

    // Wait for serial interface to settle
    delay(2000);

    // Initialize SD card interface
    Serial.println("Initializing SD card...");
    if (! sd.begin(SD_CARD_CS, SPI_HALF_SPEED)) {
        sd.initErrorHalt();
    }
    Serial.println("SD card initialized");

    // Initialize IR receiver
    irReceiver.enableIRIn();

    // Initialize 32x32 LED Matrix
    matrix.begin();
    matrix.setBrightness(DEFAULT_BRIGHTNESS);
    matrix.setColorCorrection(cc24);

    // Clear screen
    matrix.fillScreen(BLACK);
    matrix.swapBuffers();

    // Generate our color palette
    generatePalette();

    // Setup for scrolling text
    matrix.setScrollMode(wrapForward);
    matrix.setScrollSpeed(27);
    matrix.setScrollFont(font3x5);
    matrix.setScrollColor(palette[6]);
    matrix.setScrollOffsetFromEdge(18);
}

/*******************************************************************/
/***                      Main Program Loop                      ***/
/*******************************************************************/

void loop() {

    // Create new, empty image
    newImage();

    // Set parameters associated with PIXEL tool
    currentToolIndex = PIXEL;
    TOOL_DATA td = tools[currentToolIndex];
    currentToolColorIndex = td.toolColorIndex;
    currentToolParameterCount = td.numberOfParameters;
    int parameterIndex = 0;

    // Repeat until terminated
    while (true) {
        // Refresh image on screen
        refreshScreen();

        // Turn cursor back on
        positionBlockAtCursor();

        switch(waitForIRCode()) {
        case IRCODE_A:
            // User wants to pick color from color palette
            selectNewColor();
            break;

        case IRCODE_B:
            // User wants to pick new tool
            selectNewTool();
            parameterIndex = 0;
            break;

        case IRCODE_C:
            // User wants to save/load and image
            saveLoadImage();
            break;

        case IRCODE_SEL:
            // User has made a selection. What to do depends upon selected tool
            currentToolParameterCount--;

            // Have all parameters of selected tool been entered ?
            if (currentToolParameterCount != 0) {
                // No, so save position of cursor @ selection
                parameters[parameterIndex].x = cursorX;
                parameters[parameterIndex].y = cursorY;
                parameterIndex++;
            }    
            else    {
                int radius;
                switch (currentToolIndex) {
                case PIXEL:
                    // Draw pixel directly into image data
                    drawPixelIntoImage(cursorX, cursorY, currentDrawingColorIndex);
                    break;
                case LINE:
                    drawLineIntoImage(parameters[0].x, parameters[0].y, cursorX, cursorY, currentDrawingColorIndex);
                    break;
                case CIRCLE:
                    radius = sqrt(pow((parameters[0].x - cursorX), 2) + pow((parameters[0].y - cursorY), 2));
                    drawCircleIntoImage(parameters[0].x, parameters[0].y, radius, currentDrawingColorIndex);
                    break;
                case FILLED_CIRCLE:
                    radius = sqrt(pow((parameters[0].x - cursorX), 2) + pow((parameters[0].y - cursorY), 2));
                    fillCircleIntoImage(parameters[0].x, parameters[0].y, radius, currentDrawingColorIndex);
                    break;
                case RECT:
                    drawRectIntoImage(parameters[0].x, parameters[0].y, cursorX, cursorY, currentDrawingColorIndex);
                    break;
                case FILLED_RECT:
                    fillRectIntoImage(parameters[0].x, parameters[0].y, cursorX, cursorY, currentDrawingColorIndex);
                    break;
                case TRIANGLE:
                    drawTriangleIntoImage(parameters[0].x, parameters[0].y, parameters[1].x, parameters[1].y, cursorX, cursorY, currentDrawingColorIndex);
                    break;
                case FILLED_TRIANGLE:
                    fillTriangleIntoImage(parameters[0].x, parameters[0].y, parameters[1].x, parameters[1].y, cursorX, cursorY, currentDrawingColorIndex);
                    break;
                }
                // Reload the tool parameter count
                currentToolParameterCount = tools[currentToolIndex].numberOfParameters;
                parameterIndex = 0; 
            }
            break;

        case IRCODE_LEFT:
            moveCursorLeft();
            break;

        case IRCODE_RIGHT:
            moveCursorRight();
            break;

        case IRCODE_UP:
            moveCursorUp();
            break;

        case IRCODE_DOWN:
            moveCursorDown();
            break;
        }
    }
}  


