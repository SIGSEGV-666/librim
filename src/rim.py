"""This is the pure-Python library for handling .rim files. This is separate from librim.c"""
__all__ = ["RIM"]
try:
    from PIL import Image, ImagePalette
except ImportError:
    Image = None
    ImagePalette = None
try:
    import numpy as np
except ImportError:
    np = None
import struct, array
from io import IOBase, BytesIO
RIM_MAGIC = b"RAWIMG\0"
RIM_MAGIC_INDEXED = b"RAWIMGP"
RIM_MAGIC_LENGTH = len(RIM_MAGIC)
COLFMT_GRAYSCALE = 1
COLFMT_GRAYALPHA = 2
COLFMT_RGB = 3
COLFMT_RGBA = 4
VALID_COLFMTS = (
    COLFMT_GRAYSCALE,
    COLFMT_GRAYALPHA,
    COLFMT_RGB,
    COLFMT_RGBA
)
def _getfile(fp, mode='rb', open=open):
    if isinstance(fp, IOBase):
        return fp, False
    else:
        return open(fp, mode), True
def _assert_pil():
    if Image is None:
        raise RuntimeError("PIL/Pillow is not available.")
def _assert_numpy():
    if Image is None:
        raise RuntimeError("numpy is not available.")
class RIM(object):
    width, height = None, None
    colorfmt = None
    palette = None
    pixels = None
    @property
    def indexed(self):
        """Does this image have a palette?"""
        return self.palette is not None
    @property
    def num_pixels(self):
        """Number of pixels in the image"""
        return self.width*self.height
    def load(self, fp, open=open):
        """Loads a RIM file from a path or file-like object."""
        fp, autoclose = _getfile(fp, 'rb', open)
        try:
            magic = fp.read(7)
            if magic not in (RIM_MAGIC, RIM_MAGIC_INDEXED):
                raise RuntimeError("Not a valid RIM!")
            has_pal = magic == RIM_MAGIC_INDEXED
            self.palette = None
            self.width, self.height, self.colorfmt = struct.unpack("<III", fp.read(struct.calcsize("<III")))
            if self.colorfmt not in VALID_COLFMTS:
                raise ValueError("Invalid color format: "+str(self.colorfmt))
            if not has_pal:
                pxarrlen = self.num_pixels*self.colorfmt
                ogpixeldata = fp.read(pxarrlen)
                self.pixels = array.array("B", ogpixeldata)
            else:
                palsize = self.colorfmt*256
                self.palette = array.array("B", fp.read(palsize))
                self.pixels = array.array("B", fp.read(self.num_pixels))
            return self
        finally:
            if autoclose:
                fp.close()
    def save(self, fp, open=open):
        """Saves a RIM file to a path or file-like object."""
        fp, autoclose = _getfile(fp, 'wb', open)
        try:
            ispal = self.indexed
            fp.write((RIM_MAGIC_INDEXED if ispal else RIM_MAGIC))
            fp.write(struct.pack("<III", self.width, self.height, self.colorfmt))
            if ispal:
                self.palette.tofile(fp)
            self.pixels.tofile(fp)
            return self
        finally:
            if autoclose:
                fp.close()
    def palette_as_numpy_array(self):
        _assert_numpy()
        return (np.frombuffer(self.palette, dtype=np.uint8).reshape(-1, 3) if self.indexed else None)
    def pixels_as_numpy_array(self, do_palette_lookup=False):
        _assert_numpy()
        if do_palette_lookup and self.indexed:
            pal = self.palette_as_numpy_array()
            pixels_raw = np.frombuffer(self.pixels, dtype=np.uint8)
            return pal[pixels_raw].reshape(self.height, self.width, -1)
        else:
            return np.frombuffer(self.pixels, dtype=np.uint8).reshape(self.height, self.width, -1)
    def to_pil_image(self):
        _assert_pil()
        pilmode = {COLFMT_GRAYSCALE: "L", COLFMT_GRAYALPHA: "LA", COLFMT_RGB: "RGB", COLFMT_RGBA: "RGBA"}[self.colorfmt]
        if not self.indexed:
            return Image.frombuffer(pilmode, (self.width, self.height), self.pixels, "raw", pilmode, 0, 1)
        else:
            img = Image.frombuffer("P", (self.width, self.height), self.pixels, "raw")
            img.putpalette(self.palette, rawmode=pilmode)
            return img
    def convert_to_rgba(self):
        rgba_pixels = array.array("B", b'\0'*self.num_pixels*4)
        nurim = type(self)()
        nurim.width, nurim.height = self.width, self.height
        nurim.palette = None
        nurim.pixels = rgba_pixels
        nurim.colorfmt = COLFMT_RGBA
        if self.colorfmt == COLFMT_RGBA:
            rgba_pixels[:] = self.pixels
        else:
            outpixel = array.array("B", b"\0"*4)
            nullout = (0,)*4
            for i in range(self.num_pixels):
                if self.indexed:
                    invalue = self.pixels[i]
                    inpixel = self.palette[(invalue*self.colorfmt):((invalue+1)*self.colorfmt)]
                else:
                    inpixel = self.pixels[(i*self.colorfmt):((i+1)*self.colorfmt)]
                outpixel[:] = nullout
                if self.colorfmt == COLFMT_GRAYSCALE:
                    outpixel[:] = ((inpixel[0],)*3)+(255,)
                elif self.colorfmt == COLFMT_GRAYALPHA:
                    outpixel[:] = ((inpixel[0],)*3)+(inpixel[1],)
                elif self.colorfmt == COLFMT_RGB:
                    outpixel[0:3] = inpixel
                    outpixel[3] = 255
                else:
                    pass
                rgba_pixels[(i*4):((i+1)*4)] = outpixel
        return nurim