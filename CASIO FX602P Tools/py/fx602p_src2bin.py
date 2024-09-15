#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  fx602p_src2bin.py
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

#   Source file formatted with one keyvalue per line (e,g A or MinF
#   First line of the source is the 6 character programname
#	Limitation: small x can be confused with x multiply sign with src2bin. So either use CAPITALS or edit afterwards 	
#
#  Credits to :
#  Frickelfritze for creating a wavtobin program in Delphi7 years ago and providing the insights
#  http://frickelfritze.bplaced.net/casio/wavetobin/index.html

#  



	


if __name__ == '__main__':
	import sys
	import csv
	import pandas as pd
	
	
	if len(sys.argv) == 1:
		print("Command line usage: filename(no ext)")
		exit()

	argFileName=sys.argv[1]
    
	with open('res/Alpha.csv',newline='') as Alpha:
		alphaDict = (csv.DictReader(Alpha, delimiter=',',quotechar='@'))
		alphaDictList = [row for row in alphaDict]


	
	with open('res/Code.csv', newline='') as Code:		
		codeDict= (csv.DictReader(Code, delimiter=',', quotechar='@'))
		codeDictList = [row for row in codeDict]


	alphaMode=False
	with open('lst/'+argFileName +'.txt', mode='r') as listFile:
		
		with open('bin/'+argFileName +'.bin', mode='wb') as binFile: 
			
			fileName=listFile.readline()
			
			fileBytes=[]
			
			res=None

			for i in range(0,6,1):
				for sub in alphaDictList:
					if sub['alphaValue'] == fileName[i]:
						res = sub
						fileBytes.append(str(res['hexValue']))
			
			headerNibbles = []
			headerBytes = []

			for i in range(5,-1,-1):
				keyString=fileBytes[i]
				headerNibbles.append(keyString[2:3])
				headerNibbles.append(keyString[3:4])
				

			headerBytes.append('40')
			headerBytes.append('0E')
			headerBytes.append(str(headerNibbles[3])+ str(headerNibbles[1]))
			headerBytes.append(str(headerNibbles[7])+ str(headerNibbles[5]))
			headerBytes.append(str(headerNibbles[11])+ str(headerNibbles[9]))
			headerBytes.append(str(headerNibbles[2])+ str(headerNibbles[0]))
			headerBytes.append(str(headerNibbles[6])+ str(headerNibbles[4]))
			headerBytes.append(str(headerNibbles[8])+ str(headerNibbles[10]))
			
			
			for byte in headerBytes:
				binFile.write(bytes.fromhex(byte))
					
		
		
		
		
		
		
			keyValue=listFile.readline()
			
			

			while keyValue:

				keyValueStrip=keyValue.rstrip()
				
				if alphaMode:
					
					for sub in alphaDictList:
						if sub['alphaValue'] == keyValueStrip:
							res = sub
							hexString=res['hexValue']
							hexDigits=hexString[2:4]
							binFile.write(bytearray.fromhex(hexDigits))
							

							if keyValueStrip == '"':
								alphaMode=False
						
				else:
					for sub in codeDictList:
						if sub['codeValue'] == keyValueStrip:
							res = sub
							hexString=res['hexValue']
							hexDigits=hexString[2:4]
							binFile.write(bytearray.fromhex(hexDigits))
							
							if keyValueStrip=='"':
								alphaMode=True
							
				keyValue=listFile.readline()
			
			for i in range(9):
				binFile.write(bytearray.fromhex('FF'))

			binFile.write(bytearray.fromhex('00'))
			
	
	Alpha.close()
	Code.close()
	listFile.close()
	binFile.close()

	sys.exit()		


		
