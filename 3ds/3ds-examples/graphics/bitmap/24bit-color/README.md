24bit Bitmap Example
=======

This example shows a bitmap image on bottom screen.

This needs ImageMagick convert (Magick 6) or magick (Magick 7) in path. 

1. Download & install: http://www.imagemagick.org/ (If you get an option to add the application to the path make sure to check it!).

If you want to use a different image then replace brew.png in the gfx folder. Keep the same name for simplicity. If you want to change the name then you'll need to edit code with the appropriate name. That's the name of the include file - #include "brew_bgr.h" - and the "memcpy(fb, brew_bgr, brew_bgr_size);" line.


As you can see from the command in the makefile the image is clockwise rotated by 90 degrees and its B and R channels are swapped. The first operation is done because the 3DS screens are actually portrait screens rotated by 90 degrees (in a counter-clockwise direction), while the second one is done because the 3DS screens' framebuffers have a BGR888 color format, by default.