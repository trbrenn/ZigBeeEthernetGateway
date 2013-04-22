/******************************************************************************************
* DHCP_client.ino 									  *
* 											  *
* AUTHOR: Todd Brenneman. 								  *
* DATE: March 1, 2013 	   							          *
*											  *
* Hardware: Arduino Uno, Ethernet Shield, ZigBee wireless Sheild. 			  *
* 											  *
* This is my attempt at creating a gateway from a ZigBee DigiMesh network and a TCIP/IP   *
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

SoftwareSerial xbee(8,9); 		           	        //Set up SoftwareSerial to use pin 8 for recieve and pin 9 for transmit.
                                                   	        //haven't been able to get pins 1 and 2 to work with the ZigBee.
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02}; 	        //Enter a MAC address for your controller below.
byte fixedIP[] = {192, 168, 21, 71}; 		   	        //This is the fixed IP for the ethernet controller.
byte remoteServer[] = {192, 168, 21, 76}; 		        //IP address of the server.
int  remoteServerPort = 1088;                                   //The port to connect to on the remote server. 
EthernetClient client; 				   	        //Initialize the Ethernet client library

volatile byte message[125]; 				        //This is the maximum size message I've been able to push through the zigbee network.
volatile int length = 0; 	                                //this is the length we read fron the ZigBee. I'll need something similar for the ethernet side of things.
volatile byte msgType = 0x21;                                   //stores the message type.
volatile byte offset = 0x0;                                     //Storage for the offset value.
volatile byte packetID = 0x0;			                //Packet ID for the message.	

EthernetServer server = EthernetServer(1089);                   //Initialize the server that will send data to the ZigBee network.
EthernetClient serverClient;                                    //The client to recieve data from the server.
EthernetClient serrec;
/******************************************************************************************
* 											  *
* Setup(). Sets up the the basic systems or the program					  *
* 											  *
******************************************************************************************/ 
void setup() {
  Serial.begin(9600); 				   	        //start the serial library:
  Serial.println("Starting"); 			   	        //Let us know you started the serial port.
  
  if (Ethernet.begin(mac) == 0) { 		   	        // start the Ethernet connection:
    Serial.println("Failed to configure Ethernet using DHCP starting with fixed address.");
    Ethernet.begin(mac, fixedIP); 		   	        //DHCP failed so set a fixed address.
  }
  
  Serial.println(Ethernet.localIP()); 		   	        //print your local IP address to the serial port.
 
  server.begin();                                               //Start listening for messages to send to the ZigBee network.
  Serial.println("Server started");                             //Let the serial port know you started the server.
  
  xbee.begin(9600); 				   	        // set the data rate for the SoftwareSerial port
  Serial.println("XBEE Started"); 		   	        //output to the serial port.
}        

/******************************************************************************************
* 											  *
* Loop(). This is the main program.                     				  *
* 											  *
******************************************************************************************/ 
void loop() {
  byte temp = 0x0; 				   	        //Temp byte storage.
  
  			   	
    if (xbee.available()) { 			   	        //Recieve a serial input.
/*      temp = xbee.read(); 			   	        //Read the character sent.
      if(temp == 0x7e) { 			   	        //A message is follows.
	    processMsg();*/			                //Call the processMsg() function.                                        
      length = 0;                                               //If a bit arraives at the ethernet port to the ZigBee radio		        .
      temp = xbee.read(); 			  	        //Read the byte and store it in the c varible.
      if(temp == 0x7E) {                                        //If this byte is a Frame Delimiter start to process it
        length = 0;                                             //Zero out length
        message[length++] = 0x7E;                               //Make sure to keep the Frame Delimiter.
        message[length++] = xbee.read();
        delay(25);
        message[length++] = xbee.read();
        delay(25);
        int len = message[2]+8;
        Serial.print("len = ");
        Serial.println(len, HEX);
        int count = 0;
        while(count < len) {                                   //read everything from the client connect to the local server.
          message[length++] = xbee.read();                   //store the bytes in the message buffer.  
          count++;
          delay(25);          
        } 
        message[length++] = xbee.read();
      }
      processMsg(); 	
    }  
                                               
    int index = 0;                                              
    serrec = server.available();                               //Listen for connections to the local server.
    if(serrec == true){                                        //If a bit arraives at the ethernet port to the ZigBee radio		        .
      temp = serrec.read(); 			  	        //Read the byte and store it in the c varible.
      if(temp == 0x7E) {                                        //If this byte is a Frame Delimiter start to process it
        length = 0;                                             //Zero out length
        message[length++] = 0x7E;                               //Make sure to keep the Frame Delimiter.
        message[length++] = serrec.read();
        message[length++] = serrec.read();
        int len = message[2];
        int count = 0;
        while(count <= len) {                                   //read everything from the client connect to the local server.
          message[length++] = serrec.read();                   //store the bytes in the message buffer.  
          count++;          
        } 
        message[length++] = serrec.read();
        
        index = 0;
        switch (errorCheckMsg()) {                              //Check to make sure the message is in a valid format
          case 0:                                               //The message is valid                                                                //Create and zero out an index.
            while(index < length) {                             //loop through the whole message.
               xbee.write(message[index++]);                    //Write it to the ZigBee network.
            } 
            Serial.println("Message sent");                     //Let the serial monitor know everything worked.
            break;
          case 1:                                                //This packet was not a valid type.
            Serial.println("Invalid Message Type");              //Let the serial monitor know this packet was malformed.
            break;
          case 2:                                                //This packet was not the correct length.
            Serial.println("Incorrect Message Length");          //Let the serial monitor know this packet was malformed.
            break;
          case 3:                                                //The offset for this packet is wrong.
            Serial.println("Incorrect Message Offset");          //Let the serial monitor know this packet has the wrong offset
            break;
          Default:                                               //This just failed.
            Serial.println("Something Really Messed Up!");       //Let the serial monitor know this packet failed and we have no idea why.
            break;
        }
        processMsg();
      }
      else {
        Serial.println("No Start Delimiter");                    //Let the Serial monitor know the packet has no Start Delimiter.
        while(serverClient) {                                    //Flush out the trash.
          message[length++] = serrec.read();
        } 
      }  
    }
}

/*******************************************************************************************
* 											   *
* processMsg(). We recieved a message from the ZigBee network process it into XML and send *
*               it to the ethernet network server. The server will have to sort out the    *
* 		message data and do something useful with it.                              *
* 											   *
*******************************************************************************************/ 
void processMsg() {
  int index = 5;
  byte temp;
  int len = 0;
  
  if (client.connect(remoteServer,remoteServerPort)) { 	//Connect to the server at port 1088.
    Serial.println("connected"); 		   	//Output to the serial port.
  }
  else {
    Serial.println("connection failed"); 	   	//Let us know if we're not connected.
    return;
  }
  
  delay(100); 				   	        //Delays to make sure we catch each bit.
 
  Serial.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  client.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
  Serial.println("<ZigBeeMsg>"); 	   	        //output the string to the serial port for debug/testing.
  client.print("<ZigBeeMsg>"); 	   		        //output the string to the ethernet shield and send to the server. 				   	        //Delays to make sure we catch each bit.
  Serial.print("    <Length>"); 	        //output the byte to the serial port for debug/testing.
  client.print("<Length>"); 	   		        //output the byte to the ethernet shield and send to the server.
  Serial.print(message[2], HEX); 	   		        //output the byte to the serial port for debug/testing.
  client.print(message[2], HEX); 	   		        //output the byte to the ethernet shield and send to the server.
  Serial.println("</Length>"); 	   	        //output the byte to the serial port for debug/testing.
  client.print("</Length>"); 	   		        //output the byte to the ethernet shield and send to the server.
			                //Account the mesType byte removed from the stream
  Serial.print("    <MsgType>"); 	        //output the char to the serial port for debug/testing.
  client.print("<MsgType>"); 	   		        //output the char to the ethernet shield and send to the server.
  Serial.print(message[3], HEX); 	   	        //output the byte to the serial port for debug/testing.
  client.print(message[3], HEX); 	   		        //output the byte to the ethernet shield and send to the server.
  Serial.println("</MsgType>"); 	   	        //output the char to the serial port for debug/testing.
  client.print("</MsgType>"); 	   		        //output the char to the ethernet shield and send to the server.
  				                //Account the mesType byte removed from the stream.
  Serial.print("    <PacketID>"); 	        //output the char to the serial port for debug/testing.
  client.print("<PacketID>"); 	   		        //output the char to the ethernet shield and send to the server.
  Serial.print(message[4], HEX); 	   	        //output the byte to the serial port for debug/testing.
  client.print(message[4], HEX); 	   		        //output the byte to the ethernet shield and send to the server.
  Serial.println("</PacketID>"); 	   	        //output the char to the serial port for debug/testing.
  client.print("</PacketID>"); 	   		        //output the char to the ethernet shield and send to the server.
 
  Serial.print("    <MsgData>|"); 	        //output the char to the serial port for debug/testing.
  client.print("<MsgData>|"); 	   		        //output the char to the ethernet shield and send to the server.
  
  len = message[2];				   		
  while (index < len) { 		   	        //load up the message into a variable
    if(message[index] > 0x10){
      Serial.print(message[index], HEX); 	   	        //output the byte to the serial port for debug/testing.
      client.print(message[index], HEX); 	   	        //output the byte to the ethernet shield.
    }
    else {
      Serial.print("0"); 	   	        //output the byte to the serial port for debug/testing.
      client.print("0"); 	   	        //output the byte to the ethernet shield.
      Serial.print(message[index], HEX); 	   	        //output the byte to the serial port for debug/testing.
      client.print(message[index], HEX); 	   	        //output the byte to the ethernet shield.      
    }
    Serial.print("|"); 	   			        //output the seperator to the serial port for debug/testing.
    client.print("|"); 	   			        //output the seperator to the ethernet shield
    index++; 				   	        //increment index to the next element in the array.
  }
  Serial.println("</MsgData>"); 	   	        //output the char to the serial port for debug/testing.
  client.print("</MsgData>"); 	   		        //output the char to the ethernet shield and send to the server.

  int offset = message[index]; 			        //This stores the message offset.
  Serial.print("    <Offset>"); 	        //output the char to the serial port for debug/testing.
  client.print("<Offset>"); 	   		        //output the char to the ethernet shield and send to the server.
  Serial.print(offset, HEX); 	   		        //output the byte to the serial port for debug/testing.
  client.print(offset, HEX); 	   		        //output the byte to the ethernet shield and send to the server.
  Serial.println("</Offset>"); 	   	        //output the char to the serial port for debug/testing.
  client.print("</Offset>"); 	   		        //output the char to the ethernet shield and send to the server.

  Serial.println("</ZigBeeMsg>"); 		        //End of line buffer flush for the serial port.
  client.println("</ZigBeeMsg>"); 		        //End of line buffer flush for the ethernet port.

  client.println("quit");
  client.stop();
}

/******************************************************************************************
* 											  *
* errorCheckMsg(). Check the packet recieved from the Ethernet network to see if it is a  *
*                  valid ZigBee packet and report any errors found.              	  *
* 											  *
******************************************************************************************/
int errorCheckMsg() {
  int checkLength = 0;
  byte checkOffset = 0;
  
  switch(message[3]) {        //Check to see if the Frame Type is valid.
    case 0x08:               //AT Command
      break;
    case 0x09:               //Queued AT Command
      break;
    case 0x10:               //Transmit Request.
      break;
    case 0x11:               //Explicit Addressing Command Frame
      break;
    case 0x17:               //Remote Command Request
      break;
    case 0x88:               //AT Command Response
      break;
    case 0x8A:               //Modem Status
      break;
    case 0x8B:               //Transmit Status
      break;
    case 0x90:               //Receive Packet (AO=0
      break;
    case 0x91:               //Explicit Rx Indicator (AO=1
      break;
    case 0x95:               //Node Identification Indicator (AO=0
      break;
    case 0x97:               //Remote Command Response
      break;
    default:
      return 1;                     // return 1 if the frame type did not macth any valid type.
    }
  
    checkLength = length - 4;       //Find length of the data area of the packet.
    if(message[2] != checkLength) { //Check to see that the message has the correct length.
      return 2;                     //Return 2 if the message length is not correct.
    }
    
    checkLength = length - 1;                       //Make our packet length one less to shave off the offset.      
    int index = 3;                                  //Skip to the data area of the packet.
    while(index < checkLength) {                    //Step thru all the data in the packet.
      checkOffset = checkOffset + message[index++];  //Add up the offset of each byte in the data area of the packet. 
    }
    checkOffset = 0xFF & checkOffset;   //bitwise AND the offset with 0xFF to get rid anything over a byte.
    checkOffset = 0xFF - checkOffset;   //subtract offset from 0xff to get the offset.
    
    if(checkOffset != message[length - 1]) {  // check that the offsets match.
      return 3;                               //Return 3 if the offsets are bad.
    }
    
    return 0;       //Return 0 if nothing appears to be wrong.
}

/******************************************************************************************
* 											  *
* processTextMsg(). Process the basic text messages recieved by the ZigBee network.	  *
* 											  *
****************************************************************************************** 
void processTextMsg() {
  int  index = 0;
  byte header[4];

  if (client.connect(remoteServer,remoteServerPort)) { 		   	//Connect to the server at port 1088.
    Serial.println("connected"); 		   	//Output to the serial port.
    client.println("connected"); 		   	//Output to the server for logging.
  }
  else {
    Serial.println("connection failed"); 	   	//Let us know if we're not connected.
    return;
  }  

  while (index < 10) { 			   		//load up the Header variable we may use this later.
    delay(100); 			   		//Delays to make sure we catch each bit.
    header[index++] = xbee.read(); 	   		//store off the header stuff.
  }
      
  Serial.print("Packet ID: "); 	   			//output the message type to the serial port for debug/testing.
  client.print("Packet ID: "); 	   			//output to the ethernet shield and send to the server.
  Serial.print(packetID, HEX); 	   			//output the message type to the serial port for debug/testing.
  client.print(packetID, HEX); 	   			//output to the ethernet shield and send to the server.
  
  Serial.print(" Text MSg: "); 	   			//output the message type to the serial port for debug/testing.
  client.print(" Text MSg: "); 	   			//output to the ethernet shield and send to the server.
  length = length - 10; 			   	//Account for the header info we stripped off the message.
  
  index = 0; 				   		//reset index to 0 so we can step through the the list.
  while (index < length) { 		   		//load up the message into a variable
    message[index] = xbee.read(); 	   		//Write the bit in the array.
    Serial.print(message[index]); 	   		//output the char to the serial port for debug/testing.
    client.print(message[index]); 	   		//output to the ethernet shield
    index++; 				   		//increment index to the next element in the array.
    delay(25); 				  		//Delays to make sure we catch each bit.
  }

  Serial.println(" "); 			   		//End of line buffer flush for the serial port.
  client.println(" "); 			   		//End of line buffer flush for the ethernet port.

  client.stop();
}
*/

/******************************************************************************************
* 											  *
* processRemoteCommandResponseMsg(). Process the basic text messages recieved by the      *
*                                    ZigBee network.	                                  *
* 											  *
****************************************************************************************** 
void processRemoteCommandResponseMsg() {
  int index 	= 0;
  byte frameID 	= 0x0;
  byte address  = 0x0;
  byte stats 	= 0x0;
  byte cmd      = 0x0;
  
  if (client.connect(remoteServer,remoteServerPort)) { 		   	//Connect to the server at port 1088.
    Serial.println("connected"); 		   	//Output to the serial port.
    client.println("connected"); 		   	//Output to the server for logging.
  }
  else {
    Serial.println("connection failed"); 	   	//Let us know if we're not connected.
    return;
  }
  
  frameID = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
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
    length--;
    delay(25); 				  		//Delays to make sure we catch each byte.
  }
                                                        //Throw out the Responder network address.
  frameID = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  frameID = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.

  Serial.print(", cmd = ");                             //output the string to the serial port for debug/testing.
  client.print(", cmd = "); 	   	                //output the string to the ethernet shield and send to the server.
  cmd = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  Serial.print(cmd); 	   		//output the byte to the serial port for debug/testing.
  client.print(cmd); 	   		//output the byte to the ethernet shield.
  cmd = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
  delay(50);						//pause before next byte.
  Serial.print(cmd); 	   		//output the byte to the serial port for debug/testing.
  client.print(cmd); 	   		//output the byte to the ethernet shield.
  stats = xbee.read();				//read the frame ID from the stream.
  length--;						//decrement length because we read a byte from the stream.
  delay(50);
  
  index = 0;
  if(length > 0) {
    Serial.print(", Cmd Data = "); 	   		//output the byte to the serial port for debug/testing.
    client.print(", Cmd Data = "); 	   		//output the byte to the ethernet shield.
    while(index > length) {
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
  
  client.stop();
}

/******************************************************************************************
* 											  *
* processTXStatusMsg(). Process the return from a transmit.	                          *
* 											  *
****************************************************************************************** 
void processTXStatusMsg() {
  byte stats    = 0x0;
  byte frameID 	= 0x0;
  
    if (client.connect(remoteServer,remoteServerPort)) { 		   	//Connect to the server at port 1088.
    Serial.println("connected"); 		   	//Output to the serial port.
    client.println("connected"); 		   	//Output to the server for logging.
  }
  else {
    Serial.println("connection failed"); 	   	//Let us know if we're not connected.
    return;
  }
  
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
  client.stop();
}
*/



