/******************************************************************************************
* DHCP_client.ino                                                                         *
*                                                                                         *
* AUTHOR: Todd Brenneman.                                                                 *
* DATE: 3 February 3, 2013                                                                *
*                                                                                         *
* Hardware: Arduino Uno, Ethernet Shield, ZigBee wireless Sheild.                         *
*                                                                                         *
* This is my attempt at creating a gateway from a ZigBee Series 1 network and a TCIP/IP   *
* network. I want to use this to receive data from the ZigBee network and and transfer it *
* to a server which will record the data into a database. I also want to send request and *
* instructions from the server to the ZigBee network. My three requirements are that the  *
* gateway be able to keep itself connected to the TCP/IP network, send ZigBee messages    *
* from the TCP/IP network, receive data from the ZigBee network and forward it to the     *
* TCP/IP network server.                                                                  *
*                                                                                         *
******************************************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

SoftwareSerial xbee(8,9);                          //Set up SoftwareSerial to use pin 8 for recieve and pin 9 for transmit.
                                                   //haven't been able to get pins 1 and 2 to work with the ZigBee. 
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02}; //Enter a MAC address for your controller below.
byte fixedIP[] = {192, 168, 21, 71};               //This is the fixed IP for the ethernet controller.
byte server[] = {192, 168, 21, 76 };               //IP address of the server.
EthernetClient client;                             // Initialize the Ethernet client library

char message[57];                                  //This is the maximum size message I've been able to push through the zigbee network.
int lenght = 0;                                    //this is the length we read fron the ZigBee. I'll need something similar for the ethernet side of things.
byte msgType = 0x21;                               //stores the message type.
byte unknown[4];                                   //This comes in the message but I can't figure out what it is but I might need it later.
 
void setup() {
  Serial.begin(9600);                              //start the serial library:
  Serial.println("Starting");                      //Let us know you started the serial port.
  
  if (Ethernet.begin(mac) == 0) {                  // start the Ethernet connection:
    Serial.println("Failed to configure Ethernet using DHCP starting with fixed address.");    
    Ethernet.begin(mac, fixedIP);             //DHCP failed so set a fixed address.
  }
  
  if (client.connect(server, 1088)) {              //Connect to the server at port 1088.
    Serial.println("connected");                   //Output to the serial port.
    client.println("connected");                   //Output to the server for logging.
  }   
  else {
    Serial.println("connection failed");           //Let us know if we're not connected.
  }
  
  Serial.println(Ethernet.localIP());              //print your local IP address to the serial port.
  xbee.begin(9600);                                // set the data rate for the SoftwareSerial port
  Serial.println("XBEE Starting");                 //output to the serial port.
}

void loop() {
  int  index = 0;                                  //setup i for use through out loop().
  byte temp  = 0x0;                                //Temp byte storage.
  
  if (!client.connected()) {                       //Check to see if we're still connected.
    Serial.println("disconnecting.");              //tell us we lost connection.
    client.stop();                                 //Clean up the old connection.
    delay(10000);                                  //wait and try to connect agian.
    if (client.connect(server, 1088)) {            //Connect to the server at port 1088.
        Serial.println("connected");               //Output to the serial port.
        client.println("connected");               //Output to the server.
    } 
    else {
      Serial.println("connection failed");         //connection failed.
      delay(10000);                                //wait and go through the loop agian
    }    
  }                                     
  else {                                           //skip this if we're not connected.
    if (xbee.available()) {                        //Recieve a serial input.
      temp = xbee.read();                          //Read the character sent.
      if(temp == 0x7e) {                           //A message is follows.
        delay(100);                                //Delays to make sure we catch each bit.
        temp = xbee.read();                        //Throw out the first part of the length field it's always 0.
        delay(100);                                //Delays to make sure we catch each bit.
        lenght = xbee.read();                      //Get the length of the message.
        lenght = lenght - 5;                       //Account for the header info.
        delay(100);                                //Delays to make sure we catch each bit.
        msgType = xbee.read();                     //Get the type of message we're recieving.
        index = 0;                                 //reset i to 0 so we can step through the the list.
        while (index < 4) {                        //load up the unknown variable we may use this later. I need to find out what this is. 
          delay(100);                              //Delays to make sure we catch each bit.
          unknown[index++] = xbee.read();          //store off the unknown header stuff.
        }
        
        index = 0;                                 //reset i to 0 so we can step through the the list.
        while (index < lenght) {                   //load up the message into a variable 
          message[index] = xbee.read();            //Write the bit in the array.
          Serial.print(message[index]);            //out the char to the serial port for debug/testing.
          client.print(message[index]);            //output to the ethernet shield
          index++;                                 //increment i to the next element in the array.
          delay(25);                               //Delays to make sure we catch each bit.
        }
        temp = xbee.read();                        //This throws out the offset I could store if I wanted too.
        Serial.println(" ");                       //End of line buffer flush for the serial port.
        client.println(" ");                       //End of line buffer flush for the ethernet port.
      }
    }                                              //Data should be prefortamed as a ZigBee message.
    if (client.available()) {                      //If a bit arraives at the ethernet port to the ZigBee radio.
      temp = client.read();                        //Read the bye and store it in the c varible.
      Serial.print(temp);                          //out the char to the serial port for debug/testing.
      xbee.print(temp);                            //output to the ZigBee radio.
    }
  }
}

