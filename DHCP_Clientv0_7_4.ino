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
* Data Sample RX Indicator              0x92                                              *
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
  
    if(xbee.available()) {                                      //Check to see if data is available on the serial port.
      length = 0;  			   	                //Zero the length variable.
      while (xbee.available()) { 			   	//Recieve serial input while it is available.
        message[length++] = xbee.read();                        //Record the serial data from the ZigBee and store it in the message array.
        delay(25);                                              //Delay a few milliseconds just to make sure we don't drop any serial bytes.
      }
      processMsg();                                             //Process the message into XML and send it the ethernet system. 	
    } 
      
      
    int index = 0;                                              //Zero out the index.                                            
    serrec = server.available();                                //Listen for connections to the local server.
    if(serrec == true){                                         //Check if a bit has arraived at the ethernet port.
      temp = serrec.read(); 			  	        //Read the byte and store it in the temp varible.
      if(temp == 0x7E) {                                        //If this byte is a Frame Delimiter start to process the packet
        length = 0;                                             //Zero out length
        message[length++] = 0x7E;                               //Make sure to keep the Frame Delimiter.
        message[length++] = serrec.read();                      //Capture the first half of the length bytes.
        message[length++] = serrec.read();                      //Get the length byte.
        int len = message[2];                                   //Capture the message data length and save it in the len variable.
        int count = 0;                                          //Zero out the count.
        while(count <= len) {                                   //read everything while count is less then or equal to the length of the data.
          message[length++] = serrec.read();                    //store the bytes in the message array.  
          count++;                                              //increment the count.       
        } 
        message[length++] = serrec.read();                      //Capture the offset and store it in the message buffer.
        
        index = 0;                                              //Reset length to 0.
        switch (errorCheckMsg()) {                              //Check to make sure the message is in a valid format
          case 0:                                               //The message is valid                                                                
            while(index < length) {                             //loop through the whole message.
               xbee.write(message[index++]);                    //Write it to the ZigBee network.
            } 
            Serial.println("Message sent");                     //Let the serial monitor know everything worked.
            processMsg();                                       //Process the message into XML and send it the ethernet system.
            break;                                              //Stop here and leave the case statement.
          case 1:                                               //This packet was not a valid type.
            Serial.println("Invalid Message Type");             //Let the serial monitor know this packet was malformed.
            break;                                              //Stop here and leave the case statement.
          case 2:                                               //This packet was not the correct length.
            Serial.println("Incorrect Message Length");         //Let the serial monitor know this packet was malformed.
            break;                                              //Stop here and leave the case statement.
          case 3:                                               //The offset for this packet is wrong.
            Serial.println("Incorrect Message Offset");         //Let the serial monitor know this packet has the wrong offset
            break;                                              //Stop here and leave the case statement.
          Default:                                              //This just failed.
            Serial.println("Something Really Messed Up!");      //Let the serial monitor know this packet failed and we have no idea why.
            break;                                              //Stop here and leave the case statement.
        }
        
      }
      else {
        Serial.println("No Start Delimiter");                    //Let the Serial monitor know the data from the ethernet has no Start Delimiter.
        while(serverClient) {                                    //Flush out the trash.
          message[length++] = serrec.read();                     //Store whatever it is in the message array.
        } 
      }  
    }
}

/*******************************************************************************************
* 											   *
* processMsg(). We recieved a data packet and will process it into XML and send it to the  *
*               ethernet network server. The server will have to sort out the message data *
* 		 and record it into the database.                                          *
* 											   *
*******************************************************************************************/ 
void processMsg() {
  int index = 4;
  byte temp;
  int len = 0;
  int msgType = 0;
  
  if (client.connect(remoteServer,remoteServerPort)) { 	        //Connect to the server at port 1088.
    Serial.println("connected"); 		   	        //Output to the serial port.
  }
  else {
    Serial.println("connection failed"); 	   	        //Let us know if we're not connected.
    return;                                                     //Close the function and return to the main loop.
  }
 
  Serial.println("<?xml version=\"1.0\" encoding=\"UTF-8\"?>"); //Put a header on the XML data and output the string to the serial port for debug/testing.
  client.print("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");   //Put a header on the XML data and output the string to the ethernet shield and send to the server.
  Serial.println("<ZigBeeMsg>"); 	   	                //output the string to the serial port for debug/testing.
  client.print("<ZigBeeMsg>"); 	   		                //output the string to the ethernet shield and send to the server. 				   	        
  Serial.print("    <Length>"); 	                        //output the string to the serial port for debug/testing.
  client.print("<Length>"); 	   		                //output the string to the ethernet shield and send to the server.
  Serial.print(message[2], HEX); 	   		        //output the length byte to the serial port for debug/testing.
  client.print(message[2], HEX); 	   		        //output the length byte to the ethernet shield and send to the server.
  Serial.println("</Length>"); 	   	                        //output the string to the serial port for debug/testing.
  client.print("</Length>"); 	   		                //output the string to the ethernet shield and send to the server.

  msgType = message[3];			                        //record the mesType byte.
  Serial.print("    <MsgType>"); 	                        //output the string to the serial port for debug/testing.
  client.print("<MsgType>"); 	   		                //output the string to the ethernet shield and send to the server.
  Serial.print(msgType, HEX); 	   	                        //output the msgtype byte to the serial port for debug/testing.
  client.print(msgType, HEX); 	   		                //output the msgtype byte to the ethernet shield and send to the server.
  Serial.println("</MsgType>"); 	   	                //output the string to the serial port for debug/testing.
  client.print("</MsgType>"); 	   		                //output the string to the ethernet shield and send to the server.
  				                        
  if(msgType == 0x8A or msgType == 0x90 or msgType == 0x91 or msgType == 0x95 or msgType == 0x92) { //Check to see if the message type has a packet ID.
    Serial.print("    <PacketID>0</PacketID>"); 	        //We have no packet ID so output the string to the serial port for debug/testing.
    client.print("<PacketID>0</PacketID>"); 	   	        //We have no packet ID so output the string to the ethernet shield and send to the server.
  } else {  
    Serial.print("    <PacketID>"); 	                        //output the string to the serial port for debug/testing.
    client.print("<PacketID>"); 	   		        //output the string to the ethernet shield and send to the server.
    Serial.print(message[4], HEX); 	   	                //output the packet ID byte to the serial port for debug/testing.
    client.print(message[4], HEX); 	   		        //output the packet ID byte to the ethernet shield and send to the server.
    Serial.println("</PacketID>"); 	   	                //output the string to the serial port for debug/testing.
    client.print("</PacketID>"); 	   		        //output the string to the ethernet shield and send to the server.
    index = index++;                                            //adjust the point where processing of the message begins.
  }
  Serial.print("    <MsgData>|"); 	                        //output the string to the serial port for debug/testing.
  client.print("<MsgData>|"); 	   		                //output the string to the ethernet shield and send to the server.
  
  len = message[2]+3;	                                        //Not sure why this works but it does.			   		
  while (index < len) { 		   	                //Loop while there is data to be read.
    if(message[index] > 0x10){                                  //If there is ony one digit in the hex code add a leading zero.
      Serial.print(message[index], HEX); 	           	//output the byte to the serial port for debug/testing.
      client.print(message[index], HEX); 	           	//output the byte to the ethernet shield.
    }
    else {
      Serial.print("0"); 	   	                        //output the byte to the serial port for debug/testing.
      client.print("0"); 	   	                        //output the byte to the ethernet shield.
      Serial.print(message[index], HEX); 	           	//output the byte to the serial port for debug/testing.
      client.print(message[index], HEX); 	           	//output the byte to the ethernet shield.      
    }
    Serial.print("|"); 	   		        	        //output the seperator to the serial port for debug/testing.
    client.print("|"); 	   		        	        //output the seperator to the ethernet shield
    index++; 				           	        //increment index to the next element in the array.
  }
  Serial.println("</MsgData>"); 	   	                //output the string to the serial port for debug/testing.
  client.print("</MsgData>"); 	   		                //output the string to the ethernet shield and send to the server.

  int offset = message[index]; 			                //This stores the message offset.
  Serial.print("    <Offset>"); 	                        //output the string to the serial port for debug/testing.
  client.print("<Offset>"); 	   		                //output the string to the ethernet shield and send to the server.
  Serial.print(offset, HEX); 	   		                //output the byte to the serial port for debug/testing.
  client.print(offset, HEX); 	   		                //output the byte to the ethernet shield and send to the server.
  Serial.println("</Offset>"); 	   	                        //output the string to the serial port for debug/testing.
  client.print("</Offset>"); 	   		                //output the string to the ethernet shield and send to the server.

  Serial.println("</ZigBeeMsg>"); 		                //End of line buffer flush for the serial port.
  client.println("</ZigBeeMsg>"); 		                //End of line buffer flush for the ethernet port.

  client.println("quit");                                       //Tell the server to close down the ethernet connection.
  client.stop();                                                //Close the local end.
}        

/******************************************************************************************
* 											  *
* errorCheckMsg(). Check the packet recieved from the Ethernet network to see if it is a  *
*                  valid ZigBee packet and report any errors found.              	  *
* 											  *
******************************************************************************************/
int errorCheckMsg() {
  int checkLength = 0;
  int totals = 0;
  byte checkOffset = 0;
  
  switch(message[3]) {                                          //Check to see if the Frame Type is valid.
    case 0x08:                                                  //AT Command
      break;
    case 0x09:                                                  //Queued AT Command
      break;
    case 0x10:                                                  //Transmit Request.
      break;
    case 0x11:                                                  //Explicit Addressing Command Frame
      break;        
    case 0x17:                                                  //Remote Command Request
      break;
    case 0x88:                                                  //AT Command Response
      break;
    case 0x8A:                                                  //Modem Status
      break;
    case 0x8B:                                                  //Transmit Status
      break;        
    case 0x90:                                                  //Receive Packet (AO=0
      break;
    case 0x91:                                                  //Explicit Rx Indicator (AO=1
      break;
    case 0x95:                                                  //Node Identification Indicator (AO=0
      break;
    case 0x97:                                                  //Remote Command Response
      break;
    case 0x92:                                                  //Data Sample RX Indicator
      break;
    default:                                                    //I have no idea what this message type is.
      return 1;                                                 //return 1 if the frame type did not macth any valid type.
    }
  
    checkLength = length - 5;                                   //Find length of the data area of the packet.
    if(message[2] != checkLength) {                             //Check to see that the message has the correct length.
      return 2;                                                 //Return 2 if the message length is not correct.
    }
    
    checkLength = length-2;                                     //Make our packet length two less to shave off the offset.      
    int index = 3;                                              //Skip to the data area of the packet.
    while(index < checkLength) {                                //Step thru all the data in the packet.
      checkOffset = message[index++];                           //Get the data into a byte varaible.
      totals = totals + checkOffset;                            //Add up the packet bytes in the data area of the packet.
    }
    checkOffset = 0x00FF & totals;                              //bitwise AND the offset with 0xFF to get rid anything over a byte.
    checkOffset = 0xFF - checkOffset;                           //subtract offset from 0xff to get the offset.
    
    if(checkOffset != message[index]) {                         //check that the offsets match.
      return 3;                                                 //Return 3 if the offsets are bad.
    }
    
    return 0;                                                   //Return 0 if nothing appears to be wrong.
}



