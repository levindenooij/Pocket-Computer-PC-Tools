import serial
import time
import serial.tools.list_ports
import tkinter as tk
from tkinter import filedialog
import sys
txt,file_path = "",""
baud = 1200
action=""
    

def loadPort():
	i=0
	ports = serial.tools.list_ports.comports()
	for port, desc, hwid in ports:
		print("["+str(i)+"] {}: {} [{}]".format(port, desc, hwid))
		i+=1
	try:
		val = input("Enter port number [x]: ")
	except:
		print("EOF for port input occurred")

	try:
		port = ports[int(val)][0]
		return port
	except:
		print("Invalid port #\n")
		loadPort()

def connectSerial(port):
	try:
		return serial.Serial(port, baud, timeout=None, write_timeout=0.5, parity=serial.PARITY_NONE,stopbits=serial.STOPBITS_ONE,bytesize=serial.EIGHTBITS)
	except Exception as e:
		print(str(e)+"\n")
		loadPort()


def loadAndSend(ser):
	print("Put the CASIO PC in receiving mode with LOAD COM0:4,N,8,1")
	print("Select file from the dialog window:")
	root = tk.Tk()
	root.withdraw()
	file_path = filedialog.askopenfilename()
	
	with open(file_path, encoding='utf-8') as file_load:
		print("Encoding and sending: "+str(file_path).split('/')[-1], end="... ")
		data_load=''
		line_load=file_load.readline()
		while line_load != '':
			data_load = data_load+line_load +'\r\n'
			line_load=file_load.readline()
		x = ser.write(str.encode(data_load))
		print(str(x)+ "bytes loaded..",end=" ")
		time.sleep(5)
		print("Load Complete.")
		ser.close()
		file_load.close()
		
def receiveAndSave(ser):
	print("Provide filename in the dialog window:")
	root = tk.Tk()
	root.withdraw()
	file_path = filedialog.asksaveasfilename()
	
	print("Put the SHARP PC in send mode with SAVE COM0:4,N,8,1")
	
	data_save = b''
	byte_save = ser.read()
	
	while ser.inWaiting():
		data_save+=byte_save
		byte_save = ser.read()
	ser.close()
	

				
	with open(file_path,'w') as file_save:	
		x=file_save.write(data_save.decode())
	file_save.close()
	
	print (str(x)+" bytes saved..",end=" ")
	print("Save Complete")

def askAction(ser):
	try:
		action=input('L: Load, S: SAVE, E: EXIT ? ' )
		if action=="L":
			loadAndSend(ser)
		elif action =="S":
			receiveAndSave(ser)
		elif action =="E":
			exit()
		else:
			askAction(ser)
	except Exception as e:
		print(e)
	
port = loadPort()
ser = connectSerial(port)
print("\nConnected Successfully to port ", port)
askAction(ser)
	
