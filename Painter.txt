Painter - SmartMatrix / Teensy 3.1 Application

*** Turns your SmartMatrix into a IR controlled paintable canvas ***

Hardware Requirements (just like the Light Appliance):
Must have a working SmartMatrix device
Must have the SD card chip select connected to Teensy 3.1 pin 15
Must have an IR Receiver/Detector connected to Teensy 3.1 pin 18
Must use the SparkFun IR remote control

Written by: Craig A. Lindley using some code written by Louis Beaudoin (Pixelmatrix)
Version: 1.0
Last Update: 07/11/2014

The Painter sketch has the following features:

* Able to paint using a palette of 36 colors
* Supplied with 8 painting tools for drawing:
  pixels, lines, circles, filled circles, rectangles, filled rectangles,
  triangles and filled triangles
* Able to save/load paintings to/from SD card

Using the PAINTER sketch

Once Painter.ino is downloaded to your Teensy 3.1/SmartMatrix you should see a
block cursor in the center of the screen. The color in the middle is the current
drawing color and the color of the surrounding pixels indicate which drawing tool
is selected. NOTE: the block cursor is never a part of drawings you create. 
Think of it as floating above your drawing.

By default you should see a white pixel surrounded by four redish pixels. 
White is the current drawing color and the redish pixels indicate the PIXEL
drawing tool is currently selected. Each drawing tools has an associated color.

You move the block cursor with the arrow keys on the remote. If you want
to color a pixel at the position of the block cursor click the select key in the middle
of the arrow keys. If you then move the block cursor you will see a white pixel
left behind.

Changing the Drawing Color

To change the drawing color, click the A button on the remote to bring up the color
palette. Use the arrow keys on the remote to move the white selection frame to the
color you want to use and then click the select key to select that color. After color
selection is performed, you will be transitioned back to the drawing screen with
the new drawing color in the middle of the block cursor. NOTE: you cannot see the
white color selection frame when it surrounds the white palette color. Select the
black drawing color to erase pixels in a drawing.

Changing the Drawing Tool

To change the current drawing tool, click the B button on the remote. 
Ten icons representing the available drawing tools will be displayed. Upon
selection of a new drawing tool you will be immediately returned to the drawing
screen and able to use the new tool. You navigate to the tool of interest using the
arrow keys on the remote and you select the tool with the select key.

The tool in the upper left is the N or NEW tool which will erase the current
drawing allowing you to start a new one. 

The F or FILL tool will fill the drawing screen with the current drawing color.

The single redish dot represents the PIXEL tool which allows you to set a single
pixel on the screen at a time to the current drawing color.

The LINE tool will draw a line between pairs of screen coordinates you choose.

The CIRCLE tool is used to draw circles on the screen. 

The FILLED CIRCLE tool draws filled circles of the current drawing color on the screen.

The RECT tool draws a rectangle between pairs of screen coordinates you choose.

The FILLED RECT tool draws a filled rectangle on the screen.

The TRIANGLE tool draws a triangle between three screen coordinates you choose.

The FILLED TRIANGLE tools does the same but fills the triangle with the drawing
color.

Each drawing tool requires a specific number of parameters. To draw a line,
for example, requires two points to be defined in your drawing. When the LINE 
tool is selected the first time you click the select button nothing will happen. 
When you move the block cursor some where else and click the select button again,
a line will be drawn between the two points. Similarily two points are required
to define a rectangle but three points are required to define a triangle.

Saving / Loading Drawings

Clicking the C button on the remote brings up the Save/Load screen. Here you use
the remote’s arrow keys to select a filename (filenames are F1 thru F9) and then
use the A button to save the current drawing to the selected file or the B button
to load the selected file to the screen, over writing whatever drawing was there.
Clicking the C button exits this screen and returns you to the drawing screen.

As always, have fun with this SmartMatrix application. If you find bugs or add
new features, let me know. My email address is: calhjh@gmail.com

Craig Lindley




