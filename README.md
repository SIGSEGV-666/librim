# librim
A C library to handle my custom .rim file format along with some tests and tools

# About
RIM or "Raw Image" is a simple image file format I created more than a year ago for my games and whatnot.
RIM aims to be a really simple image file format that doesn't concern itself with advanced image stuff like color profiles.
RIM supports the following color modes:
    - Grayscale, 8 bpp
    - Grayscale with alpha, 16 bpp
    - RGB, 24 bpp
    - RGBA, 32 bpp
    - Indexed, 256 colors, palette can be in of any of the above color modes, 8 bpp

TODO: Add more documentation on the format.
