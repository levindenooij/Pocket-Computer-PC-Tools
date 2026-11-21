#  hp15c_dmp_to_c15.py
#  This program converts a dump file to a C15 source file. The dumpfile can be created by the HP-15C simulator or by the HP-15C calculator itself using a USB cable.
#
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
#  hp15c_dmp_to_c15.py arg=filename.
#  Required directory structure in woking directory (for convenience)
#  py: where the python programs are stored
#  txt: for hp15c keys stroke files
#  c15: for hp15c source files
#  bin15c: for hp15c token files
#  res: translation files for token translation	
#
#  Credits to :
#  Philip Houbert https://clones.phweb.me/pages/15interf.php
#  who provided input for the translation of the dump codes to C15 code and function names in his spreadsheet. 
#     

import csv
import sys

REGISTER_SIZE = 7
LAST_REGISTER_END = 2030
USER_PROGRAM_START = 496


def read_code_to_key(file_name):
    code_to_key_dict = {}
    with open(file_name, newline='') as csvfile:
        reader = csv.DictReader(csvfile)

        for row in reader:
            c15_code = ' '.join((row.get('Code') or '').split())
            code = (row.get('Code') or '').strip()
            label = (row.get('Label') or '').strip()
            operator = (row.get('Operator') or '').strip()
            alpha = (row.get('Alpha') or '').strip()
            dump_code = (row.get('Dump code') or '').strip()
            if not dump_code or not label or not c15_code:
                continue

            function_name = (
                f"{label} {operator} {alpha}"
                if operator
                else f"{label} {alpha}"
            )
            if code.startswith('42 '):
                function_name = f"f {function_name}"
            elif code.startswith('43 '):
                function_name = f"g {function_name}"
            function_name = function_name.strip()

            normalized_key = ''.join(dump_code.lower().split())
            if len(normalized_key) == 1:
                normalized_key = f"0{normalized_key}"

            code_to_key_dict[normalized_key] = (c15_code, function_name)

    return code_to_key_dict


def read_dumpcode_file(file_name):
    with open(file_name, mode='rb') as f:
        return f.read()


def iter_registers_backwards(dump_file, last_register_end):
    register_end = last_register_end
    while register_end >= REGISTER_SIZE - 1:
        separator_index = register_end + 1
        if separator_index >= len(dump_file):
            raise ValueError(f"Register separator at address {separator_index} is outside the dump")
        if dump_file[separator_index] != 0:
            break

        register_start = register_end - REGISTER_SIZE + 1
        yield register_start, dump_file[register_start:register_end + 1]
        register_end = register_start - 2


def get_user_program_bytes(dump_file):
    program_bytes = []
    for register_start, register in iter_registers_backwards(dump_file, LAST_REGISTER_END):
        if register_start < USER_PROGRAM_START:
            break
        program_bytes.extend(
            (register_start + offset, byte)
            for offset, byte in enumerate(register)
        )

    while program_bytes and program_bytes[-1][1] == 0:
        program_bytes.pop()
    return program_bytes


def convert_user_program_to_15c(dump_file, code_to_key_dict):
    program_bytes = get_user_program_bytes(dump_file)
    results = []
    index = 0
    while index < len(program_bytes):
        address, byte = program_bytes[index]
        instruction_key = format(byte, '02x')
        if instruction_key in ('af', 'bf', 'cf', 'df', 'ef', 'ff') and index + 1 < len(program_bytes):
            instruction_key += format(program_bytes[index + 1][1], '02x')
            index += 1

        instruction = code_to_key_dict.get(instruction_key)
        if instruction is None:
            raise ValueError(
                f"Address 0x{address:04X}: no C15 instruction found for dump code {instruction_key}"
            )

        c15_code, function_name = instruction
        print(f"Address 0x{address:04X}: {instruction_key} -> {function_name}")
        line_number = len(results) + 1
        results.append(
            f"{line_number:03d} {{{c15_code.rjust(12)} }} {function_name}"
        )
        index += 1

    with open('15c/list.15c', mode='w') as result_file:
        result_file.write('\n'.join(results))


def main():

    if len(sys.argv) == 1:
                print("Command line usage: filename(no ext)")
                exit()
        
    arg_file=sys.argv[1]

    code_to_key_dict = read_code_to_key('res/dumpcode_to_function.csv')
    print(code_to_key_dict)


    dump_file = read_dumpcode_file(f'bin15c/{arg_file}.bin15c')
    register_starts = [start for start, _ in iter_registers_backwards(dump_file, LAST_REGISTER_END)]
    if not register_starts:
        raise ValueError("No registers found before the dump boundary")
    print(
        f"Register sequence starts at address 0x{register_starts[-1]:04X} "
        f"({len(register_starts)} registers)"
    )

    print("User program bytes (program order)")
    program_bytes = get_user_program_bytes(dump_file)
    for index in range(0, len(program_bytes), REGISTER_SIZE):
        register_bytes = program_bytes[index:index + REGISTER_SIZE]
        print(
            f"Address 0x{register_bytes[0][0]:04X}: "
            + ' '.join(f'{byte:02X}' for _, byte in register_bytes)
        )

    convert_user_program_to_15c(dump_file, code_to_key_dict)


if __name__ == '__main__':
    main()