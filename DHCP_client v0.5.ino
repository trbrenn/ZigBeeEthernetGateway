#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

SoftwareSerial xbee(8,9);                           //Set up SoftwareSerial to use pin 8 for recieve and pin 9 for transmit.
                                                    //haven't been able to get pins 1 and 2 to work with the ZigBee. 
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};  //Enter a MAC address for your controller below.
byte fixedIP[] = {192, 168, 8, 71};                //This is the fixed IP for the ethernet controller.
byte server[] = {192, 168, 8, 76 };                //IP address of the server.
EthernetClient client;                              // Initialize the Ethernet client library

char message[57];      //This is the maximum size message I've been able to push through the zigbee network.
int lenght = 0;        //this is the length we read fron the ZigBee. I'll need something similar for the ethernet side of things.
byte msgType = 0x21;   //stores the message type.
byte unknown[4];       //This comes in the message but I can't figure out what it is but I might need it later.

void setup() {
  Serial.begin(9600);                //start the serial library:
  Serial.println("Starting");        //Let us know you started the serial port.
  
  if (Ethernet.begin(mac) == 0) {                               // start the Ethernet connection:
    Serial.println("Failed to configure Ethernet using DHCP");  // If no DHCP just use a fixed address.  
    //if (Ethernet.begin(mac, fixedIP) == 0) {                  //DHCP failed so set a fixed address.
    //  Serial.println("Failed to configure Ethernet using fixed IP address");    
        while(true) ;   //You're done give up.
    //}
  }
  
  Serial.println(Ethernet.localIP());  //print your local IP address to the serial port.
  xbee.begin(9600);                    // set the data rate for the SoftwareSerial port
  Serial.println("XBEE Starting");     //output to the serial port.

  if (client.connect(server, 1088)) {  //Connect to the server at port 1088.
    Serial.println("connected");       //Output to the serial port.
    client.println("connected");       //Output to the server.
  } 
  else {
    Serial.println("connection failed");
  }
}

void loop() {
  int i = 0;                          //setup i for use through out loop().

  if (xbee.available()) {             //Recieve a serial input.
    byte t = xbee.read();             //Read the character sent.
    if(t == 0x7e) {                   //A message is follows.
      delay(100);                     //Delays to make sure we catch each bit.
      t = xbee.read();                //Throw out the first part of the length field it's always 0.
      delay(100);                     //Delays to make sure we catch each bit.
      lenght = xbee.read();           //Get the length of the message.
      lenght = lenght - 5;            //Account for the header info.
      delay(100);                     //Delays to make sure we catch each bit.
      msgType = xbee.read();          //Get the type of message we're recieving.
      i = 0;                          //reset i to 0 so we can step through the the list.
      while (i < 4) {                 //load up the unknown variable we may use this later. I need to find out what this is. 
        delay(100);                   //Delays to make sure we catch each bit.
        unknown[i++] = xbee.read();   //store off the unknown header stuff.
       }
      
      i = 0;                          //reset i to 0 so we can step through the the list.
      while (i < lenght) {            //load up the message into a variable 
        message[i] = xbee.read();     //Write the bit in the array.
        Serial.print(message[i]);     //out the char to the serial port for debug/testing.
        client.print(message[i]);     //output to the ethernet shield
        i++;                          //increment i to the next element in the array.
        delay(25);                    //Delays to make sure we catch each bit.
      }
      byte t = xbee.read();           //This throws out the offset I could store if I wanted too.
      Serial.println(" ");            //End of line buffer flush for the serial port.
      client.println(" ");            //End of line buffer flush for the ethernet port.
    }
  }                                   //Data should be prefortamed as a ZigBee message.
  if (client.available()) {           //If a bit arraives at the ethernet port to the ZigBee radio.
    char c = client.read();           //Read the bye and store it in the c varible.
    Serial.print(c);                  //out the char to the serial port for debug/testing.
    xbee.print(c);                    //output to the ZigBee radio.
  }
}

