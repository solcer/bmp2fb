#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import division
import struct
import sys
  
  
  
class BMPError(Exception):
    """Error for problems with BMP file."""
    pass
  
  
class BMP(object):
    """BMP image.
    
    A `BMP` object can load and save very simple BMP images.  It is very
    restrictive; just uncompressed 24 bits per pixel BMPs can be loaded.
    
    HEADER_FMT : `str`
	Format in `struct` notation of the beginning of the BMP header
	structure including all relevant fields.
    HEADER_SIZE : `int`
	Size of the header structure described by `HEADER_FMT` in bytes.
    signature : `str` of length two
	BMP signature.  Must be ``BM``.
    file_size : `int`
	Size of the complete image file including header(s) and image data.
    data_offset : `int`
	Offset into the file where the image data starts.
    info_size : `int`
	Size of the info header structure.  It's 40 bytes for most BMPs.
    width, height : `int`
	Dimensions of the image in pixels.
    planes : `int`
	No idea!?  Has to be 1.
    bit_count : `int`
	Number of bits per pixel.
    compression : `int`
	Compression method.  0 means no compression.
    data_size : `int`
	Size of the (compressed) data in bytes.  If no compression is used
	this field may be set to 0.
    remaining_header : `str`
	Data between first part of the header and the image data.
    bytes_per_row : `int`
	Number of bytes per pixel row in the image data.
    data : `str`
	The image data.
    """
    HEADER_FMT = '< 2s i 4x i iii hh ii'
    HEADER_SIZE = struct.calcsize(HEADER_FMT)
    
    def __init__(self, fileobj):
	"""Create a `BMP` object from the content of `fileobj`.
	
	Raises `BMPError` if the BMP in `fileobj` is not an uncompressed
	24 bits per pixel BMP.
	"""
	(self.signature,
	  self.file_size,
	  self.data_offset,
	  self.info_size,
	  self.width,
	  self.height,
	  self.planes,
	  self.bit_count,
	  self.compression,
	  self.data_size
	) = struct.unpack(self.HEADER_FMT, fileobj.read(self.HEADER_SIZE))
	
	if self.signature != 'BM':
	    raise BMPError('not a BMP')
	if self.planes != 1:
	    raise BMPError('image has more than one plane')
	if self.bit_count != 24:
	    raise BMPError('not a 24 bpp image')
	if self.compression != 0:
	    raise BMPError('compressed image')
	
	self.remaining_header = fileobj.read(  self.data_offset
					      - self.HEADER_SIZE)
	self.data = fileobj.read()
	
	self.bytes_per_row = None
	self._calculate_bytes_per_row()
  
    def _calculate_bytes_per_row(self):
	"""Calculate the bytes per row.
	
	Row data in BMP images is padded to 32 bit boundaries.
	"""
	self.bytes_per_row = ((self.width * self.bit_count - 1) // 32) * 4 + 4
    
    def iter_rows(self):
	"""Iterate over the data of the rows."""
	for i in xrange(self.height):
	    offset = i * self.bytes_per_row
	    yield self.data[offset:offset+self.bytes_per_row]
    
    def _iter_pixels(self, rowdata):
	"""Iterate over RGB values in `rowdata`.
	
	Returns an iterator over tuples with red, green and blue values as
	`int`\s between 0 and 255.
	"""
	for i in xrange(self.width):
	    yield map(ord, rowdata[i*3:i*3+3])
    
    def convert_to_16_bit(self):
	"""Convert image to 16 bits per pixel."""
	assert self.bit_count in (24, 16)
	if self.bit_count == 16:
	    return
	#
	# Convert image data.
	#
	new_data = list()
	for rowdata in self.iter_rows():
	    new_row = list()
	    for red, green, blue in self._iter_pixels(rowdata):
		new_row.append(  ((blue >> 3) << 10)
				+ ((green >> 3) << 5)
				+  (red >> 3))
	    new_data.append(new_row)
	#
	# Adjust header information and pack image data.
	#
	self.bit_count = 16
	self.data_size = 0
	self._calculate_bytes_per_row()
	row_fmt = (  'H' * self.width
		    + 'x' * (self.bytes_per_row // 2 - self.width))
	self.data = ''.join(struct.pack(row_fmt, *row) for row in new_data)
	self.file_size = self.data_offset + len(self.data)
    
    @classmethod
    def load(cls, filename):
	"""Load a BMP file and return a `BMP` object."""
	bmp_file = open(filename, 'rb')
	result = cls(bmp_file)
	bmp_file.close()
	return result
    
    def save(self, filename):
	"""Save `BMP` object to a file."""
	bmp_file = open(filename, 'wb')
	header = struct.pack(self.HEADER_FMT,
			      self.signature,
			      self.file_size,
			      self.data_offset,
			      self.info_size,
			      self.width,
			      self.height,
			      self.planes,
			      self.bit_count,
			      self.compression,
			      self.data_size)
	bmp_file.write(header)
	bmp_file.write(self.remaining_header)
	bmp_file.write(self.data)
	bmp_file.close()
  
  
def main():
    """Main function."""
    if len(sys.argv) < 3:
	print 'usage: bmp16.py input.bmp output.bmp'
	sys.exit()
    input_name, output_name = sys.argv[1:3]
    try:
	bmp = BMP.load(input_name)
	bmp.convert_to_16_bit()
	bmp.save(output_name)
    except BMPError, error:
	print error
  
  
if __name__ == '__main__':
    main()

