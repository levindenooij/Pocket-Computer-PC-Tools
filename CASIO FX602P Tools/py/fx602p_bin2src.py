#!/usr/bin/env python
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
#  Source file formatted with one keyvalue per line
#  First line of the source is the 6 character programname 
#  Limitation: small x can be confused with x multiply sign with src2bin. So either use CAPITALS or edit afterwards 
#
#  Credits to :
#  Frickelfritze for creating a wavtobin program in Delphi7 years ago and providing the insights
#  http://frickelfritze.bplaced.net/casio/wavetobin/index.html# kcs_encode.py
#  



	



if __name__ == '__main__':
	import sys
	import csv

	if len(sys.argv) == 1:
		print("Command line usage: filename(no ext)")
		exit()

	argFileName=sys.argv[1]
	print(argFileName)

  
    
	with open('res/Alpha.csv',newline='') as Alpha:
		alphaDict = (csv.DictReader(Alpha, delimiter=',',quotechar='@'))
		alphaDictList = [row for row in alphaDict]
		
	with open('res/Code.csv', newline='') as Code:		
		codeDict= (csv.DictReader(Code, delimiter=',', quotechar='@'))
		codeDictList = [row for row in codeDict]
		
	with open('bin/'+ argFileName +'.bin', mode='rb') as binFile: 
		
		with open('lst/' + argFileName+'.txt', mode='w') as listFile:	
		
			fileBytes=binFile.read();

			headerNibbles = []
			fileName = []

			for i in range(2,8,1):
				keyString=(hex(fileBytes[i]))[2:].zfill(2).upper()
				headerNibbles.append(keyString[0:1])
				headerNibbles.append(keyString [1:2])

			
			fileName.append('0x' + str(headerNibbles[10])+ str(headerNibbles[4])) 
			fileName.append('0x' + str(headerNibbles[11])+ str(headerNibbles[5]))
			fileName.append('0x' + str(headerNibbles[8])+ str(headerNibbles[2]))
			fileName.append('0x' + str(headerNibbles[9])+ str(headerNibbles[3]))
			fileName.append('0x' + str(headerNibbles[6])+ str(headerNibbles[0]))
			fileName.append('0x' + str(headerNibbles[7])+ str(headerNibbles[1]))

	

			

			res=None

			for i in range(0,6,1):
				keyString= fileName[i]
				for sub in alphaDictList:

					if sub['hexValue'] == keyString:
						res = sub
						listFile.write(res['alphaValue'])
						
			listFile.write('\n')
			
			alphaMode=False
			for keyValue in fileBytes[8:]:
				keyString='0x' + (hex(keyValue))[2:].zfill(2).upper()
				
				if keyString=='0xFF':
					exit()
				endString=""
				res=None

				if alphaMode:
		
					for sub in alphaDictList:

						if sub['hexValue'] == keyString:
							res = sub
							
							listFile.write(res['alphaValue'])
							listFile.write('\n')
							if (keyString == '0x2B'): 
								print('\n')
								alphaMode=False
				
				else:
	
					for sub in codeDictList:
			
						if sub['hexValue'] == keyString:
							res = sub
							
							listFile.write(res['codeValue'])
							listFile.write('\n')
							if (keyString == '0x2B'): 
								print('\n')
								alphaMode=False
							
				
					if (keyString == '0x2B'):
						alphaMode=True
	Alpha.close()
	Code.close()
	listFile.close()
	binFile.close()

	sys.exit()
