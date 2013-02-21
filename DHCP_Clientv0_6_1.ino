/******************************************************************************************
* DHCP_client.ino 									  *
* 											  *
* AUTHOR: Todd Brenneman. 								  *
* DATE: 3 February 3, 2013 								  *
*											  *
* Hardware: Arduino Uno, Ethernet Shield, ZigBee wireless Sheild. 			  *
* 											  *
* This is my attempt at creating a gateway from a ZigBee Series 1 network and a TCIP/IP   *
* network. I want to use this to receive data from the ZigBee network and and transfer it *
* to a server which will record the data into a database. I also want to send request and *
* instructions from the server to the ZigBee network. My three requirements are that the  *
* gateway be able to keep itself connected to the TCP/IP network, send ZigBee messages    *
* from the TCP/IP network, receive data from the ZigBee network and forward it to the 	  *
* TCP/IP network server. 								  *
* 											  *
* API Frame Names 			API ID						  *
* -----------------------------------------------					  *
* AT Command 				0x08						  *
* AT Command - Queue Parameter Value 	0x09						  *
* Transmit Request 			0x10						  *
* Explicit Addressing Command Frame 	0x11						  *
* Remote Command Request 		0x17						  *
* AT Command Response 			0x88						  *
* Modem Status 				0x8A						  *
* Transmit Status 			0x8B						  *
* Receive Packet (AO=0) 		0x90						  *
* Explicit Rx Indicator (AO=1) 		0x91						  *
* Node Identification Indicator (AO=0) 	0x95						  *
* Remote Command Response 		0x97						  *
* 											  *
******************************************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

SoftwareSerial xbee(8,9); 		           	//Set up SoftwareSerial to use pin 8 for recieve and pin 9 for transmit.
                                                   	//haven't been able to get pins 1 and 2 to work with the ZigBee.
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02}; 	//Enter a MAC address for your controller below.
byte fixedIP[] = {192, 168, 21, 71}; 		   	//This is the fixed IP for the ethernet controller.
byte server[] = {192, 168, 21, 76}; 		   	//IP address of the server.
EthernetClient client; 				   	//Initialize the Ethernet client library

volatile char message[75]; 				//This is the maximum size message I've been able to push through the zigbee network.
volatile int lenght = 0; 	                        //this is the length we read fron the ZigBee. I'll need something similar for the ethernet side of things.
volatile byte msgType = 0x21;                           //stores the message type.
volatile byte offset = 0x0;				//Storage for the offset value.

/******************************************************************************************
* 											  *
* Setup(). Sets up the the basic systems or the program					  *
* 											  *
******************************************************************************************/ 
void setup() {
  Serial.begin(9600); 				   	//start the serial library:
  Serial.println("Starting"); 			   	//Let us know you started the serial port.
  
  if (Ethernet.begin(mac) == 0) { 		   	// start the Ethernet connection:
    Serial.println("Failed to configure Ethernet using DHCP starting with fixed address.");
    Ethernet.begin(mac, fixedIP); 		   	//DHCP failed so set a fixed address.
  }
  
  if (client.connect(server, 1088)) { 		   	//Connect to the server at port 1088.
    Serial.println("connected"); 		   	//Output to the serial port.
    client.println("connected"); 		   	//Output to the server for logging.
  }
  else {
    Serial.println("connection failed"); 	   	//Let us know if we're not connected.
  }
  
  Serial.println(Ethernet.localIP()); 		   	//print your local IP address to the serial port.
  xbee.begin(9600); 				   	// set the data rate for the SoftwareSerial port
  Serial.println("XBEE Starting"); 		   	//output to the serial port.
}

/******************************************************************************************
* 											  *
* Loop(). This is the main program.                     				  *
* 											  *
******************************************************************************************/ 
void loop() {
  byte temp = 0x0; 				   	//Temp byte storage.
  
  if (!client.connected()) { 			   	//Check to see if we're still connected.
    Serial.println("disconnecting."); 		   	//tell us we lost connection.
    client.stop(); 				   	//Clean up the old connection.
    delay(10000); 				   	//wait and try to connect agian.
    if (client.connect(server, 1088)) { 	   	//Connect to the server at port 1088.
        Serial.println("connected"); 		   	//Output to the serial port.
        client.println("connected"); 		   	//Output to the server.
    }
    else {
      Serial.println("connection failed"); 	   	//connection failed.
      delay(10000); 				   	//wait and go through the loop agian
    }
  }
  else { 					   	//skip this if we're not connected.
    if (xbee.available()) { 			   	//Recieve a serial input.
      temp = xbee.read(); 			   	//Read the character sent.
      if(temp == 0x7e) { 			   	//A message is follows.
        delay(100); 				   	//Delays to make sure we catch each bit.
        temp = xbee.read(); 			   	//Throw out the first part of the length field it's always 0.
        delay(100); 				   	//Delays to make sure we catch each bit.
        lenght = xbee.read(); 			   	//Get the length of the message.
        delay(100); 				   	//Delays to make sure we catch each bit.
        msgType = xbee.read(); 			   	//Get the type of message we're recieving.
	lenght--;				        //Account the mesType byte removed from the stream.
	
	switch(msgType) {				//Now I know what the message type is. Send the processing to the correct function.

	  case 0x81:					//Simple text message to be processed.
	    processTextMsg();				//Call the processTextMsg() function.
	    break;

	  case 0x89:					//Simple text message to be processed.
	    processTXStatusMsg();			//Call the processTextMsg() function.
	    break;

	  case 0x97:					//Response to a command.
	    processRemoteCommandResponseMsg();			//Call the processResponseMsg() function.
	    break;

	  default:					//Discard any other API frame types that are not being used.
	    processUnknownMsg();			//Call the processUnknownMsg() function.
	    break;

	offset = xbee.read(); 			   	//This stores the message offset.
      } 	
    }
   } 						   	//Data should be prefortamed as a ZigBee message.
    if (client.available()) { 			   	//If a bit arraives at the ethernet port to the ZigBee radio.
      temp = client.read(); 			  	//Read the bye and store it in the c varible.
      Serial.print(temp); 			   	//out the char to the serial port for debug/testing.
      xbee.print(temp); 			   	//output to the ZigBee radio.
    }
  }
}


/******************************************************************************************
* 											  *
* processTextMsg(). Process the basic text messages recieved by the ZigBee network.	  *
* 											  *
******************************************************************************************/ 
void processTextMsg() {
  int  index = 0;
  byte header[4];

  while (index < 4) { 			   		//load up the Header variable we may use this later.
    delay(100); 			   		//Delays to make sure we catch each bit.
    header[index++] = xbee.read(); 	   		//store off the header stuff.
  }
      
  Serial.print("Text MSg: "); 	   			//output the message type to the serial port for debug/testing.
  client.print("Text MSg: "); 	   			//output to the ethernet shield and send to the server.
  lenght = lenght - 4; 			   		//Account for the header info we stripped off the message.
  
  index = 0; 				   		//reset index to 0 so we can step through the the list.
  while (index < lenght) { 		   		//load up the message into a variable
    message[index] = xbee.read(); 	   		//Write the bit in the array.
    Serial.print(message[index]); 	   		//output the char to the serial port for debug/testing.
    client.print(message[index]); 	   		//output to the ethernet shield
    index++; 				   		//increment index to the next element in the array.
    delay(25); 				  		//Delays to make sure we catch each bit.
  }

  Serial.println(" "); 			   		//End of line buffer flush for the serial port.
  client.println(" "); 			   		//End of line buffer flush for the ethernet port.
}

/******************************************************************************************
* 											  *
* processUnknownMsg(). I don't recognize the message type so we'll clear the buffer and   *
*  		       dump the data to the outputs so we can figure out what it is and   *
*                      how to process the data.						  *
* 											  *
******************************************************************************************/ 
void processUnknownMsg() {
  int index = 0;
      
  Serial.print("Unk Msg Type: "); 	   		//output the string to the serial port for debug/testing.
  client.print("Unk Msg Type: "); 	   		//output the string to the ethernet shield and send to the server.
  Serial.print(msgType, HEX); 	   			//output the byte to the serial port for debug/testing.
  client.print(msgType, HEX); 	   			//output the byte to the ethernet shield and send to the server.
  Serial.print(" Data: |"); 	   			//output the char to the serial port for debug/testing.
  client.print(" Data: |"); 	   			//output the char to the ethernet shield and send to the server.
  				   		
  while (index < lenght) { 		   		//load up the message into a variable
    message[index] = xbee.read(); 	   		//Write the byte in the array.
    Serial.print(message[index], HEX); 	   		//output the byte to the serial port for debug/testing.
    client.print(message[index], HEX); 	   		//output the byte to the ethernet shield.
    Serial.print("|"); 	   				//output the seperator to the serial port for debug/testing.
    client.print("|"); 	   				//output the seperator to the ethernet shield
    index++; 				   		//increment index to the next element in the array.
    delay(25); 				  		//Delays to make sure we catch each byte.
  }

  Serial.println("|"); 			   		//End of line buffer flush for the serial port.
  client.println("|"); 			   		//End of line buffer flush for the ethernet port.
}

/******************************************************************************************
* 											  *
* processRemoteCommandResponseMsg(). Process the basic text messages recieved by the      *
*                                    ZigBee network.	                                  *
* 											  *
******************************************************************************************/ 
void processRemoteCommandResponseMsg() {
  int index 	= 0;
  byte frameID 	= 0x0;
  byte address  = 0x0;
  byte stats 	= 0x0;
  byte cmd      = 0x0;
  
  frameID = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.

  Serial.print("Remote Cmd Response Msg: Frame ID = "); //output the string to the serial port for debug/testing.
  client.print("Remote Cmd Response Msg: Frame ID = "); 	   	//output the string to the ethernet shield and send to the server.
  Serial.print(frameID, HEX); 	   			//output the frameID to the serial port for debug/testing.
  client.print(frameID, HEX); 	   			//output the frameID to the ethernet shield and send to the server.
  Serial.print(", Address = "); //output the string to the serial port for debug/testing.
  client.print(", Address = "); 	   	//output the string to the ethernet shield and send to the server.
  
  index = 0;
  while(index < 8) {
    address = xbee.read();	   		        //Write the byte in the array.
    Serial.print(address, HEX); 	   		//output the byte to the serial port for debug/testing.
    client.print(address, HEX); 	   		//output the byte to the ethernet shield.
    index++; 				   		//increment index to the next element in the array.
    lenght--;
    delay(25); 				  		//Delays to make sure we catch each byte.
  }
                                                        //Throw out the Responder network address.
  frameID = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  frameID = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.

  Serial.print(", cmd = ");                             //output the string to the serial port for debug/testing.
  client.print(", cmd = "); 	   	                //output the string to the ethernet shield and send to the server.
  cmd = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  Serial.print(cmd); 	   		//output the byte to the serial port for debug/testing.
  client.print(cmd); 	   		//output the byte to the ethernet shield.
  cmd = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  Serial.print(cmd); 	   		//output the byte to the serial port for debug/testing.
  client.print(cmd); 	   		//output the byte to the ethernet shield.
  stats = xbee.read();				//read the frame ID from the stream.
  lenght--;						//decrement length because we read a byte from the stream.
  delay(50);
  
  index = 0;
  if(lenght > 0) {
    Serial.print(", Cmd Data = "); 	   		//output the byte to the serial port for debug/testing.
    client.print(", Cmd Data = "); 	   		//output the byte to the ethernet shield.
    while(index > lenght) {
      cmd = xbee.read();				//read the frame ID from the stream.
      Serial.print(cmd); 	   		//output the byte to the serial port for debug/testing.
      client.print(cmd); 	   		//output the byte to the ethernet shield.
      index++;
    }
  } 

  switch(stats) {
    case 0x0:
      Serial.println(", Status = OK"); 			//End of line buffer flush for the serial port.
      client.println(", Status = OK"); 			//End of line buffer flush for the ethernet port.
      break;
    case 0x1:
      Serial.println(", Status = ERROR"); 		//End of line buffer flush for the serial port.
      client.println(", Status = ERROR"); 		//End of line buffer flush for the ethernet port.
      break;
    case 0x2:
      Serial.println(", Status = Invalid Command"); 	//End of line buffer flush for the serial port.
      client.println(", Status = Invalid Command"); 	//End of line buffer flush for the ethernet port.
      break;
    case 0x3:
      Serial.println(", Status = Invalid Parameter"); 	//End of line buffer flush for the serial port.
      client.println(", Status = Invalid Parameter"); 	//End of line buffer flush for the ethernet port.
      break;
    case 0x4:
      Serial.println(", Status = No Response"); 	//End of line buffer flush for the serial port.
      client.println(", Status = No Response"); 	//End of line buffer flush for the ethernet port.
      break;
    default:
      Serial.println(", Status = unknown?"); 		//End of line buffer flush for the serial port.
      client.println(", Status = unknown?"); 		//End of line buffer flush for the ethernet port.
      break;
  }
}

/******************************************************************************************
* 											  *
* processTXStatusMsg(). Process the return from a transmit.	                          *
* 											  *
******************************************************************************************/ 
void processTXStatusMsg() {
  byte stats    = 0x0;
  byte frameID 	= 0x0;
  
  frameID = xbee.read();
  stats = xbee.read(); 
  
  Serial.print("Tx Status Msg: Frame ID = "); 	   	//output the string to the serial port for debug/testing.
  client.print("Tx Status Msg: Frame ID = "); 	   	//output the string to the ethernet shield and send to the server.
  Serial.print(frameID, HEX); 	   			//output the frameID to the serial port for debug/testing.
  client.print(frameID, HEX); 	   			//output the frameID to the ethernet shield and send to the server.

  switch(stats) {
    case 0x0: 
      Serial.print(", Success"); 	   		//output the string to the serial port for debug/testing.
      client.print(", Success"); 	   		//output the string to the ethernet shield and send to the server.
      break;
    case 0x1: 
      Serial.print(", No ACK Rec"); 	   		//output the string to the serial port for debug/testing.
      client.print(", No ACK Rec"); 	   		//output the string to the ethernet shield and send to the server.
      break;
    case 0x2: 
      Serial.print(", CCA Failure"); 	   		//output the string to the serial port for debug/testing.
      client.print(", CCA Failure"); 	   		//output the string to the ethernet shield and send to the server.
      break;
    case 0x3: 
      Serial.print(", Purged"); 	   		//output the string to the serial port for debug/testing.
      client.print(", Purged"); 	   		//output the string to the ethernet shield and send to the server.
      break;
    default:
      Serial.print(", unknown?"); 	   		//output the string to the serial port for debug/testing.
      client.print(", unknown?"); 	   		//output the string to the ethernet shield and send to the server.
      break;
  }
}




