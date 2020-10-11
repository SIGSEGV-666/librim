#!/usr/bin/env python
"""GIMP plugin for reading and writing to .rim files"""
import os, sys
from gimpfu import *
from array import array
import struct
def convert_palette_to_rgb(palette, pixelsize):
    if pixelsize == 3:
        return palette
    else:
        converted = ''
        for i in range(256):
            centry = palette[i:i+pixelsize]
            if pixelsize in (1, 2):
                color = (ord(centry[0]),)*3
            else:
                color = tuple(map(ord, centry[0:3]))
            converted += ''.join(map(chr, color))
        return converted
def load_rawimg(filename, raw_filename):
    rim_magic = b"RAWIMG\0"
    rim_magic_indexed = b"RAWIMGP"
    img = None
    origfn = filename
    try:
        origfn = filename
        if isinstance(filename, str):
            try:
                filename = filename.decode("utf-8")
            except UnicodeDecodeError:
                filename = filename.decode("latin-1")
        with open(filename, "rb") as inrim:
            magic = inrim.read(7)
            is_indexed = False
            if magic == rim_magic:
                is_indexed = False
            elif magic == rim_magic_indexed:
                is_indexed = True
            else:
                raise IOError("Not a RIM!")
            width, height, pixelsize = struct.unpack("<III", inrim.read(struct.calcsize("<III")))
            numpixels = width*height
            if not is_indexed:
                pxarrlen = numpixels*pixelsize
                ogpixeldata = inrim.read(pxarrlen)
                pixels_array = pixeldata = array("B", ogpixeldata)
                gimpcolorfmt = {1: GRAY, 3: RGB, 4: RGB}[pixelsize]
                img = gimp.Image(width, height, gimpcolorfmt)
                img.filename = origfn
                layertype = {1: GRAY_IMAGE, 3: RGB_IMAGE, 4: RGBA_IMAGE}[pixelsize]
            else:
                palsize = pixelsize*256
                palette = convert_palette_to_rgb(inrim.read(palsize), pixelsize)
                pixels_array = pixeldata = array("B", inrim.read(width*height))
                gimpcolorfmt = INDEXED
                layertype = INDEXED_IMAGE
                img = gimp.Image(width, height, gimpcolorfmt)
                img.filename = origfn
                img.colormap = palette
            layer_name = os.path.split(filename)[1]
            layer = gimp.Layer(img, layer_name, width, height, layertype, 100.0)
            layer.set_offsets(0, 0)
            img.add_layer(layer, 0)
            pdb.gimp_edit_clear(layer)
            layer.flush()
            pxrgn = layer.get_pixel_rgn(0, 0, width, height, True, False)
            pxrgn[0:width, 0:height] = pixels_array.tostring()
            layer.flush()
            layer.merge_shadow(True)
            layer.update(0, 0, width, height)
    except:
        raise
    return img
def save_rawimg(img, drawable, filename, raw_filename):
    try:
        is_indexed = img.base_type == INDEXED
        dupimg = pdb.gimp_image_duplicate(img)
        layer = pdb.gimp_image_merge_visible_layers(dupimg, CLIP_TO_IMAGE)
        pxrgn = layer.get_pixel_rgn(0, 0, layer.width, layer.height, False, False)
        if is_indexed:
            pixelSize = 3
        else:
            pixelSize = len(pxrgn[0, 0])
        pxarray = array("B", pxrgn[0:layer.width, 0:layer.height])
        if isinstance(filename, str):
            try:
                filename = filename.decode("utf-8")
            except UnicodeDecodeError:
                filename = filename.decode("latin-1")
        with open(filename, "wb") as outraw:
            if not is_indexed:
                outraw.write(b"RAWIMG\0")
            else:
                outraw.write(b"RAWIMGP")
            outraw.write(struct.pack("<III", layer.width, layer.height, pixelSize))
            if is_indexed:
                palette = img.colormap
                remaining = 768-len(palette)
                outraw.write(palette+(b'\0'*remaining))
            pxarray.tofile(outraw)
        pdb.gimp_image_delete(dupimg)
    except Exception as err:
        gimp.message("Unexpected Error: "+str(err))
def register_load_handlers():
    gimp.register_load_handler('file-rawimg-load', 'rim', '')
def register_save_handlers():
    gimp.register_save_handler('file-rawimg-save', 'rim', '')
register(
    'file-rawimg-load', #name
    'load a RIM', #description
    'load a RIM',
    'fzwl', #author
    'fzwl', #copyright
    '2019', #year
    'Raw Image Data',
    None,
    [   #input args. Format (type, name, description, default [, extra])
        (PF_STRING, "filename", "The name of the file", None),
        (PF_STRING, "raw_filename", "The name of the file", None)
    ],
    [(PF_IMAGE, 'image', 'Output image')], #results. Format (type, name, description)
    load_rawimg, #callback
    on_query = register_load_handlers,
    menu = '<Load>'
)

register(
    'file-rawimg-save', #name
    'save a RIM', #description
    'save a RIM',
    'fzwl', #author
    'fzwl', #copyright
    '2019', #year
    'Raw Image Data',
    'GRAY, RGB, RGB*',
    [   #input args. Format (type, name, description, default [, extra])
        (PF_IMAGE, "image", "Input image", None),
        (PF_DRAWABLE, "drawable", "Input drawable", None),
        (PF_STRING, "filename", "The name of the file", None),
        (PF_STRING, "raw_filename", "The name of the file", None),
    ],
    [], #results. Format (type, name, description)
    save_rawimg, #callback
    on_query = register_save_handlers,
    menu = '<Save>'
)


main()
        

