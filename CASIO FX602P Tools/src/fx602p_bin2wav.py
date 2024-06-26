#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  fx602p_bin2wav.py
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

"""
Takes the contents of any file and encodes it into a Kansas
City Standard WAV file, that when played will upload data via the
cassette tape input on various vintage home computers. See
http://en.wikipedia.org/wiki/Kansas_City_standard
"""

import wave

# A few global parameters related to the encoding

FRAMERATE = 9600       # Hz
ONES_FREQ = 2400       # Hz (per KCS)
ZERO_FREQ = 1200       # Hz (per KCS)
AMPLITUDE = 128      # Amplitude of generated square waves
CENTER    = 128       # Center point of generated waves

# Create a single square wave cycle of a given frequency
def make_square_wave(freq,framerate):
	n = int(framerate/freq/2)
	return bytearray([CENTER-AMPLITUDE//2])*n + \
		bytearray([CENTER+AMPLITUDE//2])*n

# Create the wave patterns that encode 1s and 0s
one_pulse  = make_square_wave(ONES_FREQ,FRAMERATE)*8
zero_pulse = make_square_wave(ZERO_FREQ,FRAMERATE)*4

# Take a single byte value and turn it into a bytearray representing
# the associated waveform along with the required start and stop bits.
def kcs_encode_byte(byteval):
	bitmasks = [0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80]
	# The start bit (0)
	encoded = bytearray(zero_pulse)
	# 8 data bits	
	numOnes=0
	for mask in bitmasks:
		if(byteval & mask):
			numOnes	+=1
			encoded.extend(one_pulse)
		else:
			 encoded.extend(zero_pulse)
	#Calculate parity bit and send parity bit and two stopbits

	if numOnes%2 ==0:
		encoded.extend(zero_pulse)
	else:
		encoded.extend(one_pulse)
	encoded.extend(one_pulse)
	encoded.extend(one_pulse)
	return encoded

# Write a WAV file with encoded data. leader and trailer specify the
# number of seconds of carrier signal to encode before and after the data
def kcs_write_wav(filename,data,leader,trailer):
	w = wave.open(filename,"wb")
	w.setnchannels(1)
	w.setsampwidth(1)
	w.setframerate(FRAMERATE)

	# Write the leader

	w.writeframes(one_pulse*(int(FRAMERATE/len(one_pulse))*leader))

	# Encode the actual data
	for byteval in rawdata:
		w.writeframes(kcs_encode_byte(byteval))

	# Write the trailer
	w.writeframes(one_pulse*(int(FRAMERATE/len(zero_pulse))*trailer))
	w.close()

if __name__ == '__main__':
	import sys
	if len(sys.argv) == 1:
		print("Command line usage: filename(no ext)")
		exit()
	argFileName=sys.argv[1]
		
	f=open('bin/'+argFileName+'.bin',"rb")
	data=f.read()
	rawdata = bytearray(data)
	kcs_write_wav('wav/'+argFileName+'.wav',rawdata,5,0)
