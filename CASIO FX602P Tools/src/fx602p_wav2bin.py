#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  fx602p_wav2bin.py
#  Levin de Nooij
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  General Usage 
#  fx602p_src2bin.py arg=filename.
#  Required directory structure in woking directory (for convenience)
#  src: where the python programs are stored
#  list: for fx602p source files
#  bin: for fx602p token files
#  resources: for with alpa and code text files for token translation in alpha or code mode
#  wav: for generate wav files
#
#  Credits to :
#  Frickelfritze for creating a wavtobin program in Delphi7 years ago and providing the insights
#  http://frickelfritze.bplaced.net/casio/wavetobin/index.html# kcs_encode.py
#  
# David Beazley for the code for the generic kcs encoding/decoding programs.
#(http://www.dabeaz.com)	

from collections import deque
from itertools import islice

# Generate a sequence representing sign bits
def generate_wav_sign_change_bits(wavefile):
	samplewidth = wavefile.getsampwidth()
	nchannels = wavefile.getnchannels()
	
	previous = 0

	while True:
		frames = wavefile.readframes(8192)
		if not frames:
			break

        # Extract most significant bytes from left-most audio channel
		msbytes = bytearray(frames[samplewidth-1::samplewidth*nchannels])

        # Emit a stream of sign-change bits
		for byte in msbytes:
			signbit = byte & 0x80
			yield 1 if (signbit ^ previous) else 0
			previous = signbit

# Base frequency (representing a 1)
BASE_FREQ = 2400

# Generate a sequence of data bytes by sampling the stream of sign change bits
def generate_bytes(bitstream,framerate):
	bitmasks = [0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80]

    # Compute the number of audio frames used to encode a single data bit	
	frames_per_bit = int(round(float(framerate)*8/BASE_FREQ))
	
    # Queue of sampled sign bits
	sample = deque(maxlen=frames_per_bit)     

    # Fill the sample buffer with an initial set of data
	sample.extend(islice(bitstream,frames_per_bit-1))
	sign_changes = sum(sample)
    # Look for the start bit
	for val in bitstream:
		if val:
			sign_changes += 1
		if sample.popleft():
			sign_changes -= 1
		sample.append(val)

        # If a start bit detected, sample the next 8 data bits
		if sign_changes <= 9:
			byteval = 0
			for mask in bitmasks:
				
				if sum(islice(bitstream,frames_per_bit)) >= 12:
					byteval |= mask
			yield byteval
			# Skip the parity bit and final two stop bits and refill the sample buffer
			sample.extend(islice(bitstream,2*frames_per_bit,3*frames_per_bit-1))
			sign_changes = sum(sample)

if __name__ == '__main__':
	import wave
	import sys
	import optparse

	if len(sys.argv) == 1:
		print("Command line usage: filename(no ext)")
		exit()
	argFileName=sys.argv[1]

	wf = wave.open('wav/'+argFileName+'.wav')
	sign_changes = generate_wav_sign_change_bits(wf)

	byte_stream  = generate_bytes(sign_changes, wf.getframerate())



	# Output the byte stream in 80-byte chunks with NULL stripping
	#with open('bin/'+argFileName +'.bin', mode='wb') as binFile:
	with open('bin/'+ argFileName +'.bin',mode='wb') as binFile:
		outf = sys.stdout.buffer.raw
		while True:
			buffer = bytes(islice(byte_stream,80))
			if not buffer:
				break
			outf.write(buffer)
			binFile.write(buffer)
			
		
	exit()
