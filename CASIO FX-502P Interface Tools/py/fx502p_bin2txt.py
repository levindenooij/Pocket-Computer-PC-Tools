#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
#  fx502p_bin2txt.py

import csv
import os
import sys


def load_token_table(csv_path):
    """Loads the token mappings shared by all program areas."""
    tokens = {}
    with open(csv_path, mode='r', encoding='utf-8') as f:
        reader = csv.reader(f)
        for row in reader:
            if not row or row[0].strip().startswith('#'):
                continue
            hex_key = int(row[0].strip(), 16)
            disp_code = row[1].strip() if len(row) > 1 else f"{hex_key:02X}"
            mnemonic = row[2].strip() if len(row) > 2 else disp_code
            tokens.setdefault(hex_key, (disp_code, mnemonic))
    return tokens


def decode_header_area(byte):
    """Returns a program area encoded in a header, or None."""
    if 0x00 <= byte <= 0x09:
        return byte
    if 0xB0 <= byte <= 0xB9:
        return byte & 0x0F
    return None


def parse_bin_stream(data):
    """Finds the file header and splits its 256-byte block into area programs."""
    marker_idx = next(
        (
            i for i in range(1, len(data) - 1)
            if 0xB0 <= data[i] <= 0xB9
            and (data[i - 1] >> 4) <= 9
            and (data[i - 1] & 0x0F) <= 9
            and decode_header_area(data[i + 1]) is not None
        ),
        None,
    )
    if marker_idx is None:
        raise ValueError("File header marker (0xB0-0xB9) not found in .bin file.")

    # The marker's low nibble is the final digit of the three-digit file ID.
    id_byte = data[marker_idx - 1]
    d1 = (id_byte >> 4) & 0x0F
    d2 = id_byte & 0x0F
    d3 = data[marker_idx] & 0x0F
    file_id = f"{d1}{d2}{d3}"

    header_area = decode_header_area(data[marker_idx + 1])
    prog_start = marker_idx + 2
    block = data[prog_start : prog_start + 256]
    end_idx = block.find(0xFF)
    if end_idx != -1:
        block = block[:end_idx]

    programs = []
    current_area = header_area
    current_bytes = bytearray()
    for index, byte in enumerate(block):
        area = byte if 0x00 <= byte <= 0x09 else None
        if area is None:
            current_bytes.append(byte)
            continue
        if current_bytes:
            programs.append((current_area, bytes(current_bytes)))
            current_bytes.clear()
        current_area = area

    if current_bytes:
        programs.append((current_area, bytes(current_bytes)))

    return file_id, programs


def main(args):
    if not args:
        print("Usage: python3 fx502p_bin2src.py <filename_without_ext>")
        return 1

    base_name = args[0]
    bin_path = os.path.join('bin', f"{base_name}.bin")
    csv_path = os.path.join('res', 'code.csv')
    out_path = os.path.join('txt', f"{base_name}.txt")

    with open(bin_path, 'rb') as f:
        data = f.read()

    file_id, programs = parse_bin_stream(data)

    lines = ["; Casio FX-502P Program Dump", ""]
    tokens = load_token_table(csv_path)
    for prog_area, prog_bytes in programs:
        lines.append(f"; File ID: {file_id} | Area: P{prog_area}")
        lines.append("")
        for step, byte in enumerate(prog_bytes, start=1):
            token = tokens.get(byte)
            if token is not None:
                disp_code, mnemonic = token
                lines.append(f"{step:03d}: {disp_code:<8} ; {mnemonic}")
            else:
                lines.append(f"{step:03d}: ?{byte:02X}      ; UNKNOWN_TOKEN (0x{byte:02X})")
        lines.append("")

    output_text = "\n".join(lines) + "\n"
    print(output_text)

    os.makedirs('list', exist_ok=True)
    with open(out_path, 'w', encoding='utf-8') as out_f:
        out_f.write(output_text)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
