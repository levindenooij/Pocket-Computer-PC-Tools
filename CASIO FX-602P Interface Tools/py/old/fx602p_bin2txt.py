#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Convert FX-602P data, program, and all-file BIN images to readable text.

import csv
import sys

FOOTER = bytes([0xFF]) * 8
MAX_LINE_LENGTH = 40
FILE_TYPES = {
	(0x40, 0x40): 'M-register data file',
	(0x04, 0x0E): 'program file',
	(0x40, 0x0E): 'password-protected program file',
	(0x44, 0x4E): 'all-file',
	(0x40, 0x4E): 'password-protected all-file',
	(0x40, 0x1F): 'all-file (assumed type)',
}


def load_table(path, value_column):
	with open(path, newline='', encoding='utf-8') as file:
		return {
			int(row['hexValue'], 16): row[value_column]
			for row in csv.DictReader(file, delimiter=',', quotechar='@')
		}


def load_newline_table(path):
	with open(path, newline='', encoding='utf-8') as file:
		return {
			int(row['hexValue'], 16): row['newLine'] == 'x'
			for row in csv.DictReader(file, delimiter=',', quotechar='@')
		}


def wrap_text(text, width=MAX_LINE_LENGTH):
	lines = []
	line = ''
	for word in text.split():
		if line and len(line) + 1 + len(word) > width:
			lines.append(line)
			line = ''
		if len(word) > width:
			if line:
				lines.append(line)
				line = ''
			while len(word) > width:
				lines.append(word[:width])
				word = word[width:]
		line = f'{line} {word}'.strip()
	if line:
		lines.append(line)
	return lines


def decode_filename(file_bytes, alpha_table):
	if file_bytes[2] & 0x0F == 0x0A and file_bytes[4:8] == FOOTER[:4]:
		digits = (
			file_bytes[3] >> 4,
			file_bytes[3] & 0x0F,
			file_bytes[2] >> 4,
		)
		if any(digit > 9 for digit in digits):
			raise ValueError('File header has an invalid file number')
		prefix = {
			(0x40, 0x40): 'DF',
			(0x04, 0x0E): 'PF',
			(0x44, 0x4E): 'AF',
			(0x40, 0x4E): 'AF',
			(0x40, 0x1F): 'AF',
			(0x40, 0x0E): 'PF',
		}[tuple(file_bytes[:2])]
		return f'{prefix}{digits[0]}{digits[1]}{digits[2]}'

	nibbles = []
	for value in file_bytes[2:8]:
		nibbles.extend((value >> 4, value & 0x0F))

	name_codes = (
		(nibbles[10] << 4) | nibbles[4],
		(nibbles[11] << 4) | nibbles[5],
		(nibbles[8] << 4) | nibbles[2],
		(nibbles[9] << 4) | nibbles[3],
		(nibbles[6] << 4) | nibbles[0],
		(nibbles[7] << 4) | nibbles[1],
	)
	try:
		return ''.join(alpha_table[code] for code in name_codes)
	except KeyError as error:
		raise ValueError(f'Unknown filename character code 0x{error.args[0]:02X}') from error


def find_ff_runs(data, start=0):
	runs = []
	index = start
	while index < len(data):
		if data[index] != 0xFF:
			index += 1
			continue
		run_start = index
		while index < len(data) and data[index] == 0xFF:
			index += 1
		if index - run_start >= 8:
			runs.append((run_start, index))
	return runs


def split_password(payload, alpha_table):
	end = payload.find(b'\xEC')
	if end < 0:
		return None, payload

	reversed_password = payload[:end]
	if any(value not in alpha_table for value in reversed_password):
		return None, payload

	password = ''.join(alpha_table[value] for value in reversed(reversed_password))
	return password, payload[end + 1:]


def decode_program(
	program_bytes, code_table, alpha_table, code_newlines, alpha_newlines
):
	lines = []
	alpha_mode = False
	line = ''

	def flush_line():
		nonlocal line
		if line:
			lines.append(line)
			line = ''

	def append_token(token, in_alpha_mode):
		nonlocal line
		if not token:
			return
		separator = '' if in_alpha_mode or not line else ' '
		if line and len(line) + len(separator) + len(token) > MAX_LINE_LENGTH:
			flush_line()
			separator = '' if in_alpha_mode else '  '
		line += separator + token

	for value in program_bytes:
		if value == 0xFF:
			break
		table = alpha_table if alpha_mode else code_table
		token = table.get(value)
		if token is None:
			token = f'<0x{value:02X}>'
		if alpha_mode and value == 0x4F:
			token = ' '
		if value == 0x2B and not alpha_mode and code_newlines.get(value):
			flush_line()
		append_token(token, alpha_mode)
		if value == 0x2B:
			if alpha_mode:
				alpha_mode = False
				flush_line()
			else:
				alpha_mode = True
		elif (alpha_newlines if alpha_mode else code_newlines).get(value):
			flush_line()
	flush_line()
	return lines


def register_name(index):
	group, slot = divmod(index, 11)
	digit = str(slot) if slot < 10 else 'F'
	return f'M{group:X}{digit}'


def decode_register(register_bytes, register):
	b0, b1, b2, b3, b4, b5, b6, b7 = register_bytes
	nibbles = (
		b0 >> 4, b0 & 0x0F, b1 >> 4, b1 & 0x0F,
		b2 >> 4, b2 & 0x0F, b3 >> 4, b3 & 0x0F,
		b4 >> 4, b4 & 0x0F, b5 >> 4, b5 & 0x0F,
		b6 >> 4, b6 & 0x0F, b7 >> 4, b7 & 0x0F,
	)
	if any(nibble > 9 for nibble in nibbles):
		raise ValueError(f'{register} contains a non-BCD digit')
	if (b1 >> 4) != 0 or (b2 & 0x0F) != 0:
		raise ValueError(f'{register} has nonzero reserved mantissa nibbles')
	if (b7 >> 4) not in (0, 1):
		raise ValueError(f'{register} has an invalid sign nibble')

	exponent_digits = (b1 & 0x0F, b0 >> 4, b0 & 0x0F)
	exponent = (
		exponent_digits[0] * 100
		+ exponent_digits[1] * 10
		+ exponent_digits[2]
		- 100
	)
	mantissa_digits = (
		b7 & 0x0F,
		b6 >> 4, b6 & 0x0F,
		b5 >> 4, b5 & 0x0F,
		b4 >> 4, b4 & 0x0F,
		b3 >> 4, b3 & 0x0F,
		b2 >> 4,
	)
	digits = ''.join(str(digit) for digit in reversed(mantissa_digits))
	sign = '-' if (b7 >> 4) == 1 else '+'
	value = f'{sign}{digits[0]}.{digits[1:]}E{exponent:+d}'
	raw = ' '.join(f'{value:02X}' for value in register_bytes)
	return value, raw


def decode_data(data_bytes):
	if len(data_bytes) % 8:
		raise ValueError(
			f'Data segment has {len(data_bytes)} bytes; expected a multiple of 8'
		)

	lines = ['[DATA SEGMENT]']
	for index in range(0, len(data_bytes), 8):
		register = register_name(index // 8)
		value, raw = decode_register(data_bytes[index:index + 8], register)
		lines.append(f'{register} = {value}')
		lines.append(f'  [{raw}]')
	return lines


def convert(file_bytes, alpha_table, code_table, alpha_newlines, code_newlines):
	if len(file_bytes) < 8:
		raise ValueError('File is shorter than the 8-byte header')

	file_type = (file_bytes[0], file_bytes[1])
	if file_type not in FILE_TYPES:
		raise ValueError(
			f'Unsupported file type 0x{file_type[0]:02X} 0x{file_type[1]:02X}'
		)

	filename = decode_filename(file_bytes, alpha_table)
	lines = [
		filename,
	]
	lines.extend(wrap_text(
		f'File type: 0x{file_type[0]:02X} 0x{file_type[1]:02X} '
		f'({FILE_TYPES[file_type]})'
	))
	payload = file_bytes[8:]

	if file_type == (0x40, 0x40):
		footer_runs = find_ff_runs(payload)
		if not footer_runs:
			raise ValueError('Data file is missing its 8-byte FF footer')
		data_end = footer_runs[-1][0]
		lines.extend(decode_data(payload[:data_end]))
		return lines

	if file_type in ((0x04, 0x0E), (0x40, 0x0E)):
		if file_type == (0x40, 0x0E):
			password, program = split_password(payload, alpha_table)
		else:
			password, program = None, payload
		if password is not None:
			lines.append(f'Password: {password}')
		lines.append('[PROGRAM SEGMENT]')
		lines.extend(decode_program(
			program, code_table, alpha_table, code_newlines, alpha_newlines
		))
		return lines

	separator_runs = find_ff_runs(payload)
	if len(separator_runs) < 2:
		raise ValueError('All-file needs an FF segment separator and an FF footer')
	separator_start, separator_end = separator_runs[0]
	footer_start, _ = separator_runs[-1]
	if footer_start < separator_end:
		raise ValueError('All-file segments are out of order')

	program_bytes = payload[:separator_start]
	if file_type == (0x40, 0x4E):
		password, program = split_password(program_bytes, alpha_table)
	else:
		password, program = None, program_bytes
	if password is not None:
		lines.extend(wrap_text(f'Password: {password}'))
	lines.append('[PROGRAM SEGMENT]')
	lines.extend(decode_program(
		program, code_table, alpha_table, code_newlines, alpha_newlines
	))
	lines.extend(decode_data(payload[separator_end:footer_start]))
	return lines


if __name__ == '__main__':
	if len(sys.argv) != 2:
		print('Usage: fx602p_bin2txt.py filename (without extension)', file=sys.stderr)
		raise SystemExit(2)

	name = sys.argv[1]
	try:
		alpha_table = load_table('res/Alpha.csv', 'alphaValue')
		code_table = load_table('res/Code.csv', 'codeValue')
		alpha_newlines = load_newline_table('res/Alpha.csv')
		code_newlines = load_newline_table('res/Code.csv')
		with open(f'bin/{name}.bin', 'rb') as bin_file:
			lines = convert(
				bin_file.read(), alpha_table, code_table,
				alpha_newlines, code_newlines
			)
		with open(f'lst/{name}.txt', 'w', encoding='utf-8', newline='\n') as list_file:
			list_file.write('\n'.join(lines) + '\n')
	except (OSError, ValueError) as error:
		print(f'fx602p_bin2txt: {error}', file=sys.stderr)
		raise SystemExit(1)
