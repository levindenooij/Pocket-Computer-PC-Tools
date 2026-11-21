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
#  Required directory structure in working directory (for convenience)
#  src: where the python programs are stored
#  list: for fx602p source files
#  bin: for fx602p token files
#  resources: for with alpa and code text files for token translation in alpha or code mode
#  wav: for generate wav files

#   Source file formatted with one keyvalue per line (e,g A or MinF
#   First line of the source is the 6 character programname
#	Limitation: small x should be 𝒳 in order to avoid connfusion with x for multiplication	
#
#  Credits to :
#  Frickelfritze for creating a wavtobin program in Delphi7 years ago and providing the insights
#  http://frickelfritze.bplaced.net/casio/wavetobin/index.html

#  


def matching_alpha_rows(value, alpha_rows):
	return [
		row for row in alpha_rows
		if row.get('alphaValue') == value
	]


def encode_alpha_text(text, alpha_rows):
	text = text.replace('→', '->').replace('←', '<-')
	multi_character_tokens = sorted({
		row['alphaValue']
		for row in alpha_rows
		if row.get('alphaValue') and len(row['alphaValue']) > 1
	}, key=len, reverse=True)
	encoded = []
	position = 0

	while position < len(text):
		matched_token = None
		for token in multi_character_tokens:
			candidate = text[position:position + len(token)]
			if candidate == token or (
				token.casefold().startswith('ar')
				and candidate.casefold() == token.casefold()
			):
				matched_token = token
				break

		if matched_token is not None:
			matches = [
				row for row in alpha_rows
				if row.get('alphaValue') == matched_token
				or (
					matched_token.casefold().startswith('ar')
					and row.get('alphaValue') is not None
					and row['alphaValue'].casefold() == matched_token.casefold()
				)
			]
			encoded.extend(int(row['hexValue'], 16) for row in matches)
			position += len(matched_token)
			continue

		character = text[position]
		matches = matching_alpha_rows(character, alpha_rows)
		if character == ' ' and not matches:
			matches = [
				row for row in alpha_rows
				if row.get('alphaValue') in ('', ' ')
			]
		if not matches:
			raise ValueError(f'No alpha-plane mapping for {character!r}')
		encoded.extend(int(row['hexValue'], 16) for row in matches)
		position += 1

	return encoded


def encode_filename_header(filename, alpha_rows):
	filename = filename.strip()
	if not filename:
		return bytes((0x40, 0x0E, 0x0A, 0x00, 0xFF, 0xFF, 0xFF, 0xFF))
	if len(filename) > 6:
		raise ValueError('Program name must not exceed 6 characters')

	name_codes = []
	for character in filename.ljust(6):
		codes = encode_alpha_text(character, alpha_rows)
		if len(codes) != 1:
			raise ValueError(f'Program name character {character!r} is ambiguous')
		name_codes.append(codes[0])

	nibbles = []
	for code in reversed(name_codes):
		nibbles.extend((code >> 4, code & 0x0F))
	return bytes((
		0x40, 0x0E,
		(nibbles[3] << 4) | nibbles[1],
		(nibbles[7] << 4) | nibbles[5],
		(nibbles[11] << 4) | nibbles[9],
		(nibbles[2] << 4) | nibbles[0],
		(nibbles[6] << 4) | nibbles[4],
		(nibbles[8] << 4) | nibbles[10],
	))


def matching_code_rows(token, code_rows):
	token = {'Min0F': 'MinF', '×': '*'}.get(token, token)
	exact = [row for row in code_rows if row.get('codeValue') == token]
	if exact:
		return exact
	case_matches = [
		row for row in code_rows
		if row.get('codeValue')
		and row['codeValue'].casefold() == token.casefold()
	]
	if len(case_matches) == 1:
		return case_matches

	matches = []
	position = 0
	while position < len(token):
		candidates = [
			row for row in code_rows
			if row.get('codeValue')
			and token.startswith(row['codeValue'], position)
		]
		if not candidates:
			raise ValueError(f'No code-plane mapping for {token!r}')
		row = max(candidates, key=lambda candidate: len(candidate['codeValue']))
		matches.append(row)
		position += len(row['codeValue'])
	return matches


if __name__ == '__main__':
	import sys
	import csv
	
	
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
			
			fileName=listFile.readline().rstrip('\r\n')
			binFile.write(encode_filename_header(fileName, alphaDictList))

			skip_metadata = False
			for keyValue in listFile:
				line = keyValue.rstrip('\r\n').replace('“', '"').replace('”', '"')
				stripped_line = line.strip()
				if not alphaMode:
					if stripped_line == '[DATA SEGMENT]':
						raise ValueError(
							'Cannot convert a data segment to a program BIN'
						)
					if stripped_line.startswith(('File type:', 'Password:')):
						skip_metadata = True
						continue
					if skip_metadata:
						if stripped_line == '[PROGRAM SEGMENT]':
							skip_metadata = False
						continue
					if stripped_line == '[PROGRAM SEGMENT]':
						continue
				if not line and alphaMode:
					binFile.write(bytes(encode_alpha_text(' ', alphaDictList)))
					continue

				position = 0
				while position < len(line):
					if alphaMode:
						quote_position = line.find('"', position)
						segment_end = quote_position if quote_position >= 0 else len(line)
						segment = line[position:segment_end]
						if segment:
							binFile.write(bytes(encode_alpha_text(segment, alphaDictList)))
						if quote_position < 0:
							break
						binFile.write(bytes(encode_alpha_text('"', alphaDictList)))
						alphaMode = False
						position = quote_position + 1
						continue

					if line[position].isspace():
						position += 1
						continue
					if line[position] == '"':
						rows = matching_code_rows('"', codeDictList)
						binFile.write(bytes(int(row['hexValue'], 16) for row in rows))
						alphaMode = True
						position += 1
						continue

					token_end = position
					while (
						token_end < len(line)
						and not line[token_end].isspace()
						and line[token_end] != '"'
					):
						token_end += 1
					token = line[position:token_end]
					rows = matching_code_rows(token, codeDictList)
					binFile.write(bytes(int(row['hexValue'], 16) for row in rows))
					position = token_end
			
			for i in range(9):
				binFile.write(bytearray.fromhex('FF'))

			binFile.write(bytearray.fromhex('00'))
			
	
	Alpha.close()
	Code.close()
	listFile.close()
	binFile.close()

	sys.exit()		


		
