#  hp15c_key_to_c15.py
#  This program converts a simple key stroke file to a C15 source file. 
#  The key stroke file is a text file with the key strokes in the order they are pressed. 
#  The C15 source file is a formatted file  file that can be loaded into the VoyagerCalc HP-15C simulator.
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
#  hp15c_key_to_c15.py arg=filename.
#  Required directory structure in woking directory (for convenience)
#  py: where the python programs are stored
#  txt: for hp15c keystroke files
#  15c: for hp15c formatted source files
#  bin15c: for hp15c memory files
#  res: translation files for token translation	
#
#  Credits to :
#  Philip Houbert https://clones.phweb.me/pages/15interf.php
#  who provided input for the translation of the dump codes to C15 code and function names in his spreadsheet. 
#     
import sys
import csv

def read_key_to_number(file_name): 
    with open(file_name, newline='') as csvfile:
        reader = csv.DictReader(csvfile, fieldnames=['key', 'number'], delimiter=',', quotechar='@')
        key_to_number_dict = {row['key']: row['number'] for row in reader}
    return key_to_number_dict


def read_txt_file(file_name):                 
    with open(file_name, mode='r') as txt_file:
        file_name=txt_file.readlines()   
    return file_name    

def convert_text_to_code(file_name, key_to_number_dict):  
    file_codes=[]
    line_num=1
    for line in file_name:
        code_string = ''
        print(line)
        for key in line.split():
            print(key)
            code=key_to_number_dict.get(key)
            print(code)
            code_string+=code + " "
        print(code_string)
        line_str=str(line_num).zfill(3) +" " +"{" + code_string.strip().rjust(12) + " " + "}" + " " + line.strip()
        file_codes.append(line_str)
        line_num+=1
    
    return file_codes

def write_15c_file(file_name,file_codes):
    with open('15c/'+file_name +'.15c', mode='wb') as txt_file: 
        txt_file.write('\n'.join(file_codes).encode('utf-8'))  # Write the codes to the .15c file
 
def main():
    if len(sys.argv) == 1:
            print("Command line usage: filename(no ext)")
            exit()
    
    arg_file=sys.argv[1]
    key_to_number_dict = read_key_to_number('res/key_to_number.csv')
    #print(key_to_number_dict)
    txt_file = read_txt_file('txt/' + arg_file + '.txt')  # Assuming you have a function to read the text file
    print(txt_file)
    file_codes=convert_text_to_code(txt_file, key_to_number_dict)
    print(file_codes)
    write_15c_file(arg_file, file_codes)  # Assuming you have a function to write the .15c file


if __name__ == '__main__':
    main()