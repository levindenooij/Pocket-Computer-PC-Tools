
import java.util.Scanner;
import javax.swing.JFileChooser;
import java.io.File;
import java.io.FileOutputStream;
import java.nio.file.Files;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.lang.System;
import java.nio.charset.Charset;
import com.fazecast.jSerialComm.SerialPort;


public class Sharpsiotransfer {

	static SerialPort activePort;
	static SerialPort[] ports = SerialPort.getCommPorts();
	static char myAction;
	static int portIndex;
	private static Charset UTF8 = Charset.forName("UTF8");
	
	public static void main(String[] args) {
		int p = 0;
		for(SerialPort port : ports) {
			System.out.print(p + ". " + port.getDescriptivePortName() + " ");
			System.out.println(port.getPortDescription());
			p++;
			}
		System.out.print("What serial port do you want me to use? ");

		Scanner scanner = new Scanner(System.in);
		try{
			portIndex = scanner.nextInt();
		} catch (Exception e) {
			System.out.println("Choose a portnumber");
		}
		scanner.nextLine();	
		activePort = ports[portIndex];
		activePort.openPort();
		activePort.setBaudRate(1200);
		activePort.setNumDataBits(8);
		activePort.setNumStopBits(SerialPort.ONE_STOP_BIT);
		activePort.setParity(SerialPort.NO_PARITY);
		activePort.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING,0,0);
	/* 	activePort.setFlowControl(SerialPort.FLOW_CONTROL_CTS_ENABLED);
		activePort.setFlowControl(SerialPort.FLOW_CONTROL_RTS_ENABLED);
		activePort.setRTS(); */

		// Get input and output streams
		InputStream in = activePort.getInputStream();
		OutputStream out = activePort.getOutputStream();	

			System.out.println("L: Load, S: SAVE, E: EXIT ?");
			myAction = scanner.next().charAt(0);
			System.out.println(myAction);
			scanner.nextLine();
        
				switch (myAction) {
					case 'L':
						// Let the user put the pocket computer in Receiving mode 

						System.out.println("Press LOAD on the SHARP PC..");
						System.out.println("Now choose a basic file to load from the PC..");
						try {
							// Choose a file on the PC
							JFileChooser o = new JFileChooser();
							o.showOpenDialog(null);
							File outPath = o.getSelectedFile();
							int outSize = (int)outPath.length();

							// read the file contents in a buffer
							byte[] outBuff = new byte[outSize];
							outBuff= Files.readAllBytes(outPath.toPath());
							String outData = new String(outBuff, 0, outSize);
							System.out.println("Data to load in SHARP: " + outData);

							// Write the buffer to the serial port plus EOF

							for (byte b: outBuff) {
								if (b!= 0X1A) out.write(b);
							}
							out.write(0X1A);

							System.out.println(outSize + " bytes have been loaded on the SHARP PC. Goodbye!");
							scanner.close();

						} catch (Exception e) {
							System.out.println(e);
						}

						break;
					case 'S':
						// Let user make data available on the serial port
						System.out.println("Press Save on the SHARP PC..");

						// Ask for file on the PC to store the basic program

						System.out.println("\n"+"Now provide a basic filename on the PC to save the data to");

						try {
													
						// Provide a file name

							JFileChooser i = new JFileChooser();
							i.showSaveDialog(null);
							File inPath = i.getSelectedFile();
							System.out.println(inPath+"\n");

						// read the serial contents in a buffer
							int inSize = in.available();
							byte[] inBuff = new byte[inSize];
							inBuff=in.readNBytes(inSize);
							String inData = new String(inBuff, 0, inSize, UTF8);
							System.out.println(" Data received from Pocket PC is \n\n");
							System.out.println(inData);
						
						// Write the buffer to the file
							Writer writer = new OutputStreamWriter(new FileOutputStream(inPath), UTF8);
 					       for (byte b:inBuff) {
								//x1A end of file
 					           if (b!= 0X1A) writer.write(b);
							}
							System.out.println("\n" +inSize + " bytes have been saved in file " + inPath +" on the the computer. Goodbye!");
 				           	writer.close();
							scanner.close();
							System.exit(0);

						} 
						catch (Exception e) {
							System.out.println (e);
							}
							
					break;
			
					case 'E':
						System.out.println("Goodbye");
						scanner.close();
						System.exit(0);
					default:
						System.out.println("Command not recognized, try again");
						break;
				}
			}
				
}