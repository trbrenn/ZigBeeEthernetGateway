#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>

SoftwareSerial xbee(8,9); // RX, TX
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
byte server[] = { 192, 168, 21, 76 };
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

void setup() {
  // start the serial library:
  Serial.begin(9600);
  Serial.println("Starting");
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // print your local IP address:
  Serial.println(Ethernet.localIP());
  // set the data rate for the SoftwareSerial port
  xbee.begin(9600);
  Serial.println("XBEE Starting");
  
  if (client.connect(server, 1088)) {
    Serial.println("connected");
    client.println("connected");
  } else {
    Serial.println("connection failed");
  }
}

void loop() {
  if (xbee.available()) {      //Recieve a serial input.
    char t = xbee.read();      //Read the character sent.
    if(t == 0x7e) {
        Serial.println(" ");
        client.println(" ");
    }
    Serial.print(t);
    client.print(t);
  }
  if (client.available()) {
    char c = client.read();
    Serial.print(c);
    xbee.print(c);
  }
}
