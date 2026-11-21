#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  fx502p_txt2bin.py

import csv
import os
import re
import sys


def load_reverse_token_table(csv_path):
    """Loads DISPLAY_CODE and MNEMONIC -> HEX_BYTE mappings from resources/code.csv."""
    lookup = {}
    with open(csv_path, mode='r', encoding='utf-8') as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].strip().startswith('#'):
                continue
            hex_val = int(row[0].strip(), 16)
            if hex_val <= 0x09:
                continue
            if len(row) > 1 and row[1].strip():
                lookup.setdefault(row[1].strip().upper(), hex_val)
            if len(row) > 2 and row[2].strip():
                lookup.setdefault(row[2].strip().upper(), hex_val)
    return lookup


def parse_source_file(src_path, lookup):
    """Parses a listing file from list/<name>.txt into header info and instruction bytes."""
    file_id = "111"
    prog_area = 0x00
    program_bytes = []
    programs = []
    area_seen = False

    with open(src_path, mode='r', encoding='utf-8') as f:
        for line_num, raw_line in enumerate(f, start=1):
            line = raw_line.strip()

            # Check header comments for File ID and Area (e.g. "; File ID: 111 | Area: P0")
            if line.startswith(';'):
                id_match = re.search(r'File\s*ID:\s*(\d{3})', line, re.IGNORECASE)
                if id_match:
                    file_id = id_match.group(1)
                area_match = re.search(r'Area:\s*P(\d)', line, re.IGNORECASE)
                if area_match:
                    if area_seen:
                        programs.append((prog_area, program_bytes))
                        program_bytes = []
                    prog_area = int(area_match.group(1)) & 0x0F
                    area_seen = True
                continue

            # Strip inline comments (everything after ';') and optional step numbers (e.g. "001: ")
            parts = line.split(';', 1)
            code_part = parts[0].strip()
            code_part = re.sub(r'^\d{1,3}:\s*', '', code_part).strip()
            if not code_part:
                continue

            key = code_part.upper()
            mnemonic = parts[1].strip().upper() if len(parts) > 1 else ''
            if mnemonic in lookup:
                program_bytes.append(lookup[mnemonic])
            elif key in lookup:
                program_bytes.append(lookup[key])
            elif re.fullmatch(r'\?[0-9A-F]{2}', key):
                # Allow raw hex escape tokens like "?8A" for unmapped opcodes
                program_bytes.append(int(key[1:], 16))
            else:
                raise ValueError(f"Line {line_num}: Unknown instruction '{code_part}'")

    if area_seen or program_bytes:
        programs.append((prog_area, program_bytes))

    for area, steps in programs:
        if len(steps) > 256:
            raise ValueError(f"Program P{area} exceeds 256-step FX-502P memory limit ({len(steps)} steps).")

    return file_id, programs


def build_bin_stream(file_id, programs):
    """Constructs the complete FX-502P tape .bin byte sequence."""
    if not programs:
        programs = [(0x00, [])]

    programs_by_area = {}
    for area, program_bytes in programs:
        if area in programs_by_area:
            raise ValueError(f"Program area P{area} appears more than once.")
        if not 0 <= area <= 9:
            raise ValueError(f"Invalid program area P{area}.")
        programs_by_area[area] = program_bytes

    block_size = sum(len(program_bytes) for _, program_bytes in programs) + len(programs) - 1
    if block_size > 256:
        raise ValueError(f"Programs exceed 256-byte FX-502P memory block ({block_size} bytes).")

    # 1. Leader tone bytes (32 bytes of 0xFF carrier)
    stream = bytearray([0xFF] * 32)

    # 2. 3-digit BCD File ID; the marker's low nibble is the final ID digit.
    d1, d2, d3 = int(file_id[0]), int(file_id[1]), int(file_id[2])
    stream.append((d1 << 4) | d2)
    stream.append(0xB0 | d3)

    # 3. Initial area byte, followed by area marker bytes between programs.
    stream.append(programs[0][0] & 0x0F)
    for index, (area, program_bytes) in enumerate(programs):
        if index:
            stream.append(area & 0x0F)
        stream.extend(program_bytes)

    # 4. Pad the combined program areas to the full 256-byte memory block.
    padding_len = 256 - block_size
    stream.extend([0xFF] * padding_len)

    # 5. Trailer / end-of-block carrier bytes (8 bytes of 0xFF)
    stream.extend([0xFF] * 8)

    return bytes(stream)


def main(args):
    if not args:
        print("Usage: python3 fx502p_src2bin.py <filename_without_ext>")
        return 1

    base_name = args[0]
    src_path = os.path.join('txt', f"{base_name}.txt")
    csv_path = os.path.join('res', 'code.csv')
    bin_path = os.path.join('bin', f"{base_name}.bin")

    lookup = load_reverse_token_table(csv_path)
    file_id, programs = parse_source_file(src_path, lookup)
    bin_data = build_bin_stream(file_id, programs)

    os.makedirs('bin', exist_ok=True)
    with open(bin_path, mode='wb') as out_f:
        out_f.write(bin_data)

    summary = ", ".join(f"P{area}: {len(steps)} steps" for area, steps in programs)
    print(f"Compiled '{src_path}' -> '{bin_path}' (ID: {file_id}, {summary}).")
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
