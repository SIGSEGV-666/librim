"""This is the pure-Python library for handling .rim files. This is separate from librim.c"""
__all__ = ["RIM"]
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