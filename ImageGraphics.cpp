/*
 * SmartMatrix Library - Methods for interacting with background layer
 *
 * Copyright (c) 2014 Louis Beaudoin (Pixelmatix)
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
 * Code from MatrixGraphics.cpp was modified substantially by Craig A. Lindley for use in the
 * Painter application. This code writes into an image data array using color indices instead
 * of writing into a backing buffer using rgb24 values.
 */

#include <arduino.h>

const int WIDTH  = 32;
const int HEIGHT = 32;

extern byte image[WIDTH][HEIGHT];

void drawPixelIntoImage(int x, int y, byte colorIndex) {

    image[x][y] = colorIndex;
}

#define SWAPint(X,Y) { int temp = X; X = Y; Y = temp; }

// x0, x1, and y must be in bounds
void drawHardwareHLine(int x0, int x1, int y, byte colorIndex) {

    for (int i = x0; i <= x1; i++) {
        image[i][y] = colorIndex;
    }
}

// x, y0, and y1 must be in bounds
void drawHardwareVLine(int x, int y0, int y1, byte colorIndex) {

    for (int i = y0; i <= y1; i++) {
        image[x][i] = colorIndex;
    }
}

void drawFastHLine(int x0, int x1, int y, byte colorIndex) {

    // make sure line goes from x0 to x1
    if (x1 < x0) {
        SWAPint(x1, x0);
    }

    // check for completely out of bounds line
    if (x1 < 0 || x0 >= WIDTH || y < 0 || y >= HEIGHT) {
        return;
    }

    // truncate if partially out of bounds
    if (x0 < 0) {
        x0 = 0;
    }

    if (x1 >= WIDTH) {
        x1 = WIDTH - 1;
    }

    drawHardwareHLine(x0, x1, y, colorIndex);
}

void drawFastVLine(int x, int y0, int y1, byte colorIndex) {

    // make sure line goes from y0 to y1
    if (y1 < y0) {
        SWAPint(y1, y0);
    }

    // check for completely out of bounds line
    if (y1 < 0 || y0 >= HEIGHT || x < 0 || x >= WIDTH) {
        return;
    }

    // truncate if partially out of bounds
    if (y0 < 0) {
        y0 = 0;
    }

    if (y1 >= HEIGHT) {
        y1 = HEIGHT - 1;
    }

    drawHardwareVLine(x, y0, y1, colorIndex);
}

void bresteepline(int x3, int y3, int x4, int y4, byte colorIndex) {

    // if point x3, y3 is on the right side of point x4, y4, change them
    if ((x3 - x4) > 0) {
        bresteepline(x4, y4, x3, y3, colorIndex);
        return;
    }

    int x = x3, y = y3, sum = x4 - x3,  Dx = 2 * (x4 - x3), Dy = abs(2 * (y4 - y3));
    int prirastokDy = ((y4 - y3) > 0) ? 1 : -1;

    for (int i = 0; i <= x4 - x3; i++) {
        drawPixelIntoImage(y, x, colorIndex);
        x++;
        sum -= Dy;
        if (sum < 0) {
            y = y + prirastokDy;
            sum += Dx;
        }
    }
}

// algorithm from http://www.netgraphics.sk/bresenham-algorithm-for-a-line
void drawLineIntoImage(int x1, int y1, int x2, int y2, byte colorIndex) {

    // if point x1, y1 is on the right side of point x2, y2, change them
    if ((x1 - x2) > 0) {
        drawLineIntoImage(x2, y2, x1, y1, colorIndex);
        return;
    }
    // test inclination of line
    // function Math.abs(y) defines absolute value y
    if (abs(y2 - y1) > abs(x2 - x1)) {
        // line and y axis angle is less then 45 degrees
        // that swhy go on the next procedure
        bresteepline(y1, x1, y2, x2, colorIndex);
        return;
    }
    // line and x axis angle is less then 45 degrees, so x is guiding
    // auxiliary variables
    int x = x1, y = y1, sum = x2 - x1, Dx = 2 * (x2 - x1), Dy = abs(2 * (y2 - y1));
    int prirastokDy = ((y2 - y1) > 0) ? 1 : -1;
    // draw line
    for (int i = 0; i <= x2 - x1; i++) {
        drawPixelIntoImage(x, y, colorIndex);
        x++;
        sum -= Dy;
        if (sum < 0) {
            y = y + prirastokDy;
            sum += Dx;
        }
    }
}

// algorithm from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
void drawCircleIntoImage(int x0, int y0, int radius, byte colorIndex) {

    int a = radius, b = 0;
    int radiusError = 1 - a;

    if (radius == 0)
        return;

    while (a >= b) {
        drawPixelIntoImage(a + x0, b + y0, colorIndex);
        drawPixelIntoImage(b + x0, a + y0, colorIndex);
        drawPixelIntoImage(-a + x0, b + y0, colorIndex);
        drawPixelIntoImage(-b + x0, a + y0, colorIndex);
        drawPixelIntoImage(-a + x0, -b + y0, colorIndex);
        drawPixelIntoImage(-b + x0, -a + y0, colorIndex);
        drawPixelIntoImage(a + x0, -b + y0, colorIndex);
        drawPixelIntoImage(b + x0, -a + y0, colorIndex);

        b++;
        if (radiusError < 0) {
            radiusError += 2 * b + 1;
        }    
        else    {
            a--;
            radiusError += 2 * (b - a + 1);
        }
    }
}

// algorithm from drawCircle rearranged with hlines drawn between points on the raidus
void fillCircleIntoImage(int x0, int y0, int radius, byte colorIndex) {

    int a = radius, b = 0;
    int radiusError = 1 - a;

    if (radius == 0) {
        return;
    }

    // only draw one line per row, skipping the top and bottom
    bool hlineDrawn = true;

    while (a >= b) {
        // this pair sweeps from horizontal center down
        drawFastHLine((a - 1) + x0, (-a + 1) + x0, b + y0, colorIndex);

        // this pair sweeps from horizontal center up
        drawFastHLine((a - 1) + x0, (-a + 1) + x0, -b + y0, colorIndex);

        if (b > 1 && !hlineDrawn) {
            drawFastHLine((b - 1) + x0, (-b + 1) + x0, a + y0, colorIndex);
            drawFastHLine((b - 1) + x0, (-b + 1) + x0, -a + y0, colorIndex);
            hlineDrawn = true;
        }

        b++;
        if (radiusError < 0) {
            radiusError += 2 * b + 1;
        } 
        else {
            a--;
            hlineDrawn = false;
            radiusError += 2 * (b - a + 1);
        }
    }
}

void drawRectIntoImage(int x0, int y0, int x1, int y1, byte colorIndex) {

    drawFastHLine(x0, x1, y0, colorIndex);
    drawFastHLine(x0, x1, y1, colorIndex);
    drawFastVLine(x0, y0, y1, colorIndex);
    drawFastVLine(x1, y0, y1, colorIndex);
}

void fillRectIntoImage(int x0, int y0, int x1, int y1, byte colorIndex) {

    for (int i = y0; i <= y1; i++) {
        drawFastHLine(x0, x1, i, colorIndex);
    }
}

void drawTriangleIntoImage(int x1, int y1, int x2, int y2, int x3, int y3, byte colorIndex) {

    drawLineIntoImage(x1, y1, x2, y2, colorIndex);
    drawLineIntoImage(x2, y2, x3, y3, colorIndex);
    drawLineIntoImage(x1, y1, x3, y3, colorIndex);
}

// Code from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void fillFlatSideTriangleInt(int x1, int y1, int x2, int y2, int x3, int y3, byte colorIndex) {

    int t1x, t2x, t1y, t2y;
    bool changed1 = false;
    bool changed2 = false;
    int8_t signx1, signx2, signy1, signy2, dx1, dy1, dx2, dy2;
    int i;
    int8_t e1, e2;

    t1x = t2x = x1; 
    t1y = t2y = y1; // Starting points

    dx1 = abs(x2 - x1);
    dy1 = abs(y2 - y1);
    dx2 = abs(x3 - x1);
    dy2 = abs(y3 - y1);

    if (x2 - x1 < 0) {
        signx1 = -1;
    } 
    else signx1 = 1;
    if (x3 - x1 < 0) {
        signx2 = -1;
    } 
    else signx2 = 1;
    if (y2 - y1 < 0) {
        signy1 = -1;
    } 
    else signy1 = 1;
    if (y3 - y1 < 0) {
        signy2 = -1;
    } 
    else signy2 = 1;

    if (dy1 > dx1) {   // swap values
        SWAPint(dx1, dy1);
        changed1 = true;
    }
    if (dy2 > dx2) {   // swap values
        SWAPint(dy2, dx2);
        changed2 = true;
    }

    e1 = 2 * dy1 - dx1;
    e2 = 2 * dy2 - dx2;

    for (i = 0; i <= dx1; i++)
    {
        drawFastHLine(t1x, t2x, t1y, colorIndex);

        while (e1 >= 0)
        {
            if (changed1)
                t1x += signx1;
            else
                t1y += signy1;
            e1 = e1 - 2 * dx1;
        }

        if (changed1)
            t1y += signy1;
        else
            t1x += signx1;

        e1 = e1 + 2 * dy1;

        /* here we rendered the next point on line 1 so follow now line 2
         * until we are on the same y-value as line 1.
         */
        while (t2y != t1y)
        {
            while (e2 >= 0)
            {
                if (changed2)
                    t2x += signx2;
                else
                    t2y += signy2;
                e2 = e2 - 2 * dx2;
            }

            if (changed2)
                t2y += signy2;
            else
                t2x += signx2;

            e2 = e2 + 2 * dy2;
        }
    }
}

// Code from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void fillTriangleIntoImage(int x1, int y1, int x2, int y2, int x3, int y3, byte colorIndex) {

    // Sort vertices
    if (y1 > y2) {
        SWAPint(y1, y2);
        SWAPint(x1, x2);
    }
    if (y1 > y3) {
        SWAPint(y1, y3);
        SWAPint(x1, x3);
    }
    if (y2 > y3) {
        SWAPint(y2, y3);
        SWAPint(x2, x3);
    }

    if (y2 == y3) {
        fillFlatSideTriangleInt(x1, y1, x2, y2, x3, y3, colorIndex);
    }

    /* check for trivial case of top-flat triangle */
    else if (y1 == y2) {
        fillFlatSideTriangleInt(x3, y3, x1, y1, x2, y2, colorIndex);
    }
    else {
        /* general case - split the triangle in a topflat and bottom-flat one */
        int16_t xtmp, ytmp;
        xtmp = (int)(x1 + ((float)(y2 - y1) / (float)(y3 - y1)) * (x3 - x1));
        ytmp = y2;
        fillFlatSideTriangleInt(x1, y1, x2, y2, xtmp, ytmp, colorIndex);
        fillFlatSideTriangleInt(x3, y3, x2, y2, xtmp, ytmp, colorIndex);
    }
}



