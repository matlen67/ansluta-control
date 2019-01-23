/*
	Ansulta Control with NodeMCU and CC2500
	
	created 24.11.2018
	by matlen67
	
	modified 23.01.2019
  by flokycool
  
	Hardware:
	  * NodeMCU Lolin V3 (az-delivery)
		* CC2500 (WLC240)  (ebay)
		
	Wired:
           NodeMCU                 CC2500        Push-Button
		
		SPI     GPIO   Board
		----------------------         --------      ------
           3.3V   (3V3)             VCC
		SLC    GPIO14  (D5)             SCLK
		MISO   GPIO12  (D6)             MISO
		MOSI   GPIO13  (D7)             MOSI
		CSx    GPIO15  (D8)             CSN
           Ground   (G)             G				     Button Pin1
                   (D3)                          Button Pin2
*/


#include "cc2500_REG.h"
#include "cc2500_VAL.h"
#include "cc2500_CMD.h"
#include <SPI.h>
#include <ESP8266WiFi.h>
#define USE_MQTT 0  //To use MQTT, install Library "PubSubClient" and switch line to 1
#define USE_BUTTON 0  //To use a local Push-Button for on/off toggle, set this to 1

#if USE_BUTTON == 1
  const int buttonPin = D3;  // If you wish to use a different pinout, change it here
  int buttonState = HIGH;   // This worked for my Buttone depending on your Button you might try LOW here
  int thisButtonState = HIGH;  // This worked for my Buttone depending on your Button you might try LOW here
  int lastButtonState = HIGH;  // This worked for my Buttone depending on your Button you might try LOW here
  unsigned long lastDebounceTime = 0;  // the time the button state last switched
  unsigned long debounceDelay = 50;    // the state must remain the same for this many millis to register the button press
#endif

#if USE_MQTT == 1
  #include <PubSubClient.h>
  //Your MQTT Broker
  const char* mqtt_server = "192.168.0.2";  //enter your mqtt server jere
  const char* mqtt_in_topic = "ansluta/light/set";  //modifiy your mqtt topic to your needs
  const char* mqtt_out_topic = "ansluta/light/status";  //modifiy your mqtt topic to your needs
#endif

#define CS 15                     // ChipSelect NodeMCU Pin15

#define Light_OFF       0x01      // Command to turn the light off
#define Light_ON_50     0x02      // Command to turn the light on 50%
#define Light_ON_100    0x03      // Command to turn the light on 100%
#define Light_PAIR      0xFF      // Command to pair a remote to the light

bool relais = 0;  //dummy relais just to remember last light state 
const boolean DEBUG = true;       // debug mode print same infos by RS232

const char* ssid     = "yourssid";
const char* password = "yourwifipassword";


#if USE_MQTT == 1
  WiFiClient espClient;
  PubSubClient client(espClient);
  bool status_mqtt = 1;
#endif

WiFiServer server(80);
String header;

/* get your ansluta address!
   -------------------------
   connect browser to webcontrol
   example: 192.168.178.130/ansluta/getAddress
   press button within 5 sec on your original ansluta remote
   is a valid address detect it will be displayed
*/ 

// pute here your AddressBytes
byte AddressByteA = 0x1A;
byte AddressByteB = 0x0B;

 
void setup(){

  #if USE_BUTTON == 1
    //pushbutton
    pinMode(buttonPin, INPUT);
  #endif

  // config ChipSelect
  pinMode(CS,OUTPUT);
     
  if(DEBUG){
    Serial.begin(115200);
    Serial.println("Debug mode");
    Serial.print("Initialisation");
  }

  // Connect to Wi-Fi network with SSID and password
  if(DEBUG){
    Serial.print("Connecting to ");
    Serial.println(ssid);
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if(DEBUG){
      Serial.print(".");
    }
  }
  // Print local IP address and start web server
  if(DEBUG){
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  #if USE_MQTT == 1  
    client.setServer(mqtt_server, 1883);
    client.setCallback(MqttCallback);
  #endif

  server.begin();

  // SPI config 
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  digitalWrite(CS,HIGH);
  
  SendStrobe(CC2500_SRES); //0x30 SRES Reset chip.
  init_CC2500();
  
  //  SendStrobe(CC2500_SPWD); //Enter power down mode    -   Not used in the prototype
  WriteReg(0x3E,0xFF);  //Maximum transmit power - write 0xFF to 0x3E (PATABLE)
  
  if(DEBUG){
    Serial.println(" - Done");
  }
  
  // end setup
}

void loop(){
  #if USE_MQTT == 1
  //MQTT
   if (!client.connected()) {
    MqttReconnect();
   }
   if (client.connected()) {
    MqttStatePublish();
   }
  client.loop();
  #endif 

  #if USE_BUTTON == 1
  //Button Press Handle
    thisButtonState = digitalRead(buttonPin);
    if (thisButtonState != lastButtonState) {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {
      if (thisButtonState != buttonState) {
        buttonState = thisButtonState;
        if (buttonState != HIGH) {  // This worked for my Buttone depending on your Button you might try LOW here
          if (relais == 1 ) {
            Switch_Off();
          } else {
            Switch_On();
          }        
        }
      }
    }
    lastButtonState = thisButtonState;
  #endif
  
  //Web Server
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    if(DEBUG){
      Serial.println("New Client.");        // print a message out in the serial port
    }
    String currentLine = "";                // make a String to hold incoming data from the client
    String adrByte = "";
    boolean adrState = false;
    int timeOut = 0;
    
    while (client.connected()) {            // loop while the client's connected
      timeOut ++;
      if (timeOut >= 500000){               // dirty workaround because ClientTimeout not Work!
        client.stop();                      // Chrome keep alive the connection and block other connection (Fhem,Firefox)
        }
        
      if (client.available()) {             // if there's bytes to read from the client,
        
        char c = client.read();             // read a byte, then
        if(DEBUG){
          Serial.write(c);                    // print it out the serial monitor
        }
        header += c;
        
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { width: 10em; margin-left: auto; margin-right: auto; background-color: #195B6A; border: solid 1px transparent;border-radius: 4px; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>-- NodeMCU --</h1>");
            client.println("<body><h2>Ansluta Control</h2>");
            // turn the light on/off
            if (header.indexOf("GET /ansluta/50") >= 0) {
              if(DEBUG){
                Serial.println("Ansluta 50% ");
              }
              SendCommand(AddressByteA,AddressByteB, Light_ON_50);
            } else if (header.indexOf("GET /ansluta/100") >= 0) {
              if(DEBUG){
                Serial.println("Ansluta 100%");
              }
              Switch_On();
            } else if (header.indexOf("GET /ansluta/0") >= 0) {
              if(DEBUG){
                Serial.println("Ansluta 0%");
              }
              Switch_Off();
            } else if (header.indexOf("GET /ansluta/Pair") >= 0) {
              if(DEBUG){
                Serial.println("Ansluta Pair");
              }
              SendCommand(AddressByteA,AddressByteB, Light_PAIR);
            } else if (header.indexOf("GET /ansluta/getAddress") >= 0) {
              if(DEBUG){
                Serial.println("Ansluta getAddress");
              }
              adrState = true;
              client.println("<br>");
              client.println("<p>Listen for adressbyte...</p>");
              client.println("<p>Press button on original remote!</p>");
              //client.println("</body></html>");
              client.println();

              delay(10);
              client.flush();
              //client.stop();
              adrByte = ReadAddressBytes();
            
            }
            
            if(relais == 0){
              client.println("<p>Status <b>off</b></p>");
            }else{
              client.println("<p>Status <b>on</b></p>");
            }
                        
            if(adrState == false){
              client.println("<br>");
              client.println("<p></p>");
              client.println("<p><a href=\"/ansluta/50\"><button class=\"button\"> 50% </button></a></p>");
            
              client.println("<br>");            
              client.println("<p></p>");
              client.println("<p><a href=\"/ansluta/100\"><button class=\"button\">100%</button></a></p>");

              client.println("<br>");
              client.println("<p></p>");
              client.println("<p><a href=\"/ansluta/0\"><button class=\"button\">off</button></a></p>");
            }else{
              client.println("<br>");
              client.println("<p>Your Ansluta address: " + adrByte + "</p>");
              adrState = false;
              adrByte = "";   
            }
            
            client.println("</body></html>");
           
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    //delay(1);
    client.stop();

    if(DEBUG){
      Serial.println("Client disconnected.");
      Serial.println("");
    }
  }
}

#if USE_MQTT == 1
void MqttCallback(char* topic, byte* payload, unsigned int length) {
  // Ausgabe payload
  if(DEBUG){
    Serial.print("MQTT Payload: ");
    for (int i=0; i<length; i++){
      Serial.print(char(payload[i]));
    }
    Serial.println("");
  }
  // Switch on
  if ((char)payload[0] == 'o' && (char)payload[1] == 'n' ) {
    Switch_On();
  //Switch off
  } else {
    Switch_Off();
  }
}

void MqttReconnect() {
  String clientID = "Ansluta_"; // 13 chars
  clientID += WiFi.macAddress();//17 chars

  while (!client.connected()) {
    if(DEBUG){
      Serial.println("Connect to MQTT-Broker");
    }
    if (client.connect(clientID.c_str())) {
      if(DEBUG){
        Serial.print("connected as clientID:");
        Serial.println(clientID);
      }
      //publish ready
      client.publish(mqtt_out_topic, "mqtt client ready");
      //subscribe in topic
      client.subscribe(mqtt_in_topic);
    } else {
      if(DEBUG){
        Serial.print("failed: ");
        Serial.print(client.state());
        Serial.println(" try again...");
      }
      delay(5000);
    }
  }
}

void MqttStatePublish() {
  if (relais == 1 and not status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "on");
      if(DEBUG){
        Serial.println("MQTT publish: on");
      }
     }
  if (relais == 0 and status_mqtt)
     {
      status_mqtt = relais;
      client.publish(mqtt_out_topic, "off");
      if(DEBUG){
        Serial.println("MQTT publish: off");
      }
     }
}
#endif

void Switch_On(void) {
  relais = 1;
  SendCommand(AddressByteA,AddressByteB, Light_ON_100);
//  startAt = 0;
}
void Switch_Off(void) {
  relais = 0;
  SendCommand(AddressByteA,AddressByteB, Light_OFF);
//  stopAt = 0;
}
 
String ReadAddressBytes(){     //Read Address Bytes From a remote by sniffing its packets wireless
   byte tries=0;
   boolean AddressFound = false;
   String A = "";
   String B = "";
   if(DEBUG){
    Serial.print("Listening for an Address");
   }

       
   while((tries<200)&&(!AddressFound)){ 
       
      SendStrobe(CC2500_SRX);       // 0x34 Enable RX.
      WriteReg(REG_IOCFG1,0x01);    // REG_IOCFG1 = 0x01 Switch MISO to output if a packet has been received or not
      delay(20);

      byte PacketLength = ReadReg(CC2500_FIFO);
      byte recvPacket[PacketLength];
         
      if ( PacketLength >= 1) {   
             
        if(DEBUG){
          Serial.println();
          Serial.print("Packet received: ");
          Serial.print(PacketLength,DEC);
          Serial.println(" bytes");
        }
        
        if(PacketLength <= 8) {                       //A packet from the remote cant be longer than 8 bytes
          for(byte i = 1; i <= PacketLength; i++){    //Read the received data from CC2500
            recvPacket[i] = ReadReg(CC2500_FIFO);
            if(DEBUG){
              if(recvPacket[i]<0x10){Serial.print("0");}
              Serial.print(recvPacket[i],HEX);
            }
          }
          if(DEBUG){
            Serial.println();
          }
         }
          
          byte start=0;
          while((recvPacket[start]!=0x55) && (start < PacketLength)){   //Search for the start of the sequence
            start++;
          }
          if(recvPacket[start+1]==0x01 && recvPacket[start+5]==0xAA){   //If the bytes match an Ikea remote sequence
            AddressFound = true;
            AddressByteA = recvPacket[start+2];                         // Extract the addressbytes
            AddressByteB = recvPacket[start+3];
            if(DEBUG){
              Serial.print("Address Bytes found: ");
              if(AddressByteA<0x10){Serial.print("0");}
              Serial.print(AddressByteA,HEX);
              if(AddressByteB<0x10){Serial.print("0");}
              Serial.println(AddressByteB,HEX);
            }

            if(AddressByteA<0x10){A = "0";}
            A += String(AddressByteA,HEX);
             
            if(AddressByteA<0x10){B = "0";}
            B += String(AddressByteB,HEX);

            return A + B;
          } 
        SendStrobe(CC2500_SIDLE);      // Exit RX / TX
        SendStrobe(CC2500_SFRX);       // Flush the RX FIFO buffer
      } 
      tries++;  
   }
   if(DEBUG){
    Serial.println(" - Done");
   }

   return "";
}


byte ReadReg(byte addr){
  
  addr = addr + 0x80;			          // set r/w bit (bit7=1 read, bit7=0 write)
  
  digitalWrite(CS,LOW);
  delayMicroseconds(1);             // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  byte x = SPI.transfer(addr);
  delay(10);
  byte y = SPI.transfer(0);
  delayMicroseconds(1);
  digitalWrite(CS,HIGH);
  
  return y;  
}


void SendStrobe(byte strobe){
  
  digitalWrite(CS,LOW);
  delayMicroseconds(1);             // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  SPI.write(strobe);
  digitalWrite(CS,HIGH); 
  delayMicroseconds(2);
 
}


void SendCommand(byte AddressByteA, byte AddressByteB, byte Command){

         
    for(byte i=0;i<50;i++){       	//Send 50 times
      
      SendStrobe(CC2500_SFTX);      // 0x3B
      SendStrobe(CC2500_SIDLE);     // 0x36
      
      digitalWrite(CS,LOW);
      delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
      
      SPI.transfer(0x7F);           // activate burst data
      delayMicroseconds(2);
      
      SPI.transfer(0x06);           // send 6 data bytes
      delayMicroseconds(2);
     
      SPI.transfer(0x55);           // ansluta data byte 1
      delayMicroseconds(2);
      
      SPI.transfer(0x01);           // ansluta data byte 2         
      delayMicroseconds(2);
      
      SPI.transfer(AddressByteA);   // ansluta data address byte A
      delayMicroseconds(2);
      
      SPI.transfer(AddressByteB);   // ansluta data address byte B
      delayMicroseconds(2);
      
      SPI.transfer(Command);        // ansluta data command 0x01=Light OFF 0x02=50% 0x03=100% 0xFF=Pairing
      delayMicroseconds(2);
      
      SPI.transfer(0xAA);           // ansluta data byte 6
      delayMicroseconds(2);

      digitalWrite(CS,HIGH);        // end transfer
             
      SendStrobe(CC2500_STX);      // 0x35  transmit data in TX
      
      delayMicroseconds(1600);
      
    }  

   
    //delay(20);
    //SendStrobe(CC2500_SPWD);      // 0x39  cc2500 sleep
    //delayMicroseconds(2);
}


void WriteReg(byte addr, byte value){
  
  digitalWrite(CS,LOW);
  delayMicroseconds(1);         // can't wait for digitalRead(MISO)==HIGH! Don't work in SPI mode
  SPI.transfer(addr);
  delayMicroseconds(1);
  SPI.transfer(value);
  delayMicroseconds(1);
  digitalWrite(CS,HIGH);  
}


void init_CC2500(){
	
  WriteReg(REG_IOCFG2,VAL_IOCFG2);
  WriteReg(REG_IOCFG0,VAL_IOCFG0);
  WriteReg(REG_PKTLEN,VAL_PKTLEN);
  WriteReg(REG_PKTCTRL1,VAL_PKTCTRL1);
  WriteReg(REG_PKTCTRL0,VAL_PKTCTRL0);
  WriteReg(REG_ADDR,VAL_ADDR);
  WriteReg(REG_CHANNR,VAL_CHANNR);
  WriteReg(REG_FSCTRL1,VAL_FSCTRL1);
  WriteReg(REG_FSCTRL0,VAL_FSCTRL0);
  WriteReg(REG_FREQ2,VAL_FREQ2);
  WriteReg(REG_FREQ1,VAL_FREQ1);
  WriteReg(REG_FREQ0,VAL_FREQ0);
  WriteReg(REG_MDMCFG4,VAL_MDMCFG4);
  WriteReg(REG_MDMCFG3,VAL_MDMCFG3);
  WriteReg(REG_MDMCFG2,VAL_MDMCFG2);
  WriteReg(REG_MDMCFG1,VAL_MDMCFG1);
  WriteReg(REG_MDMCFG0,VAL_MDMCFG0);
  WriteReg(REG_DEVIATN,VAL_DEVIATN);
  WriteReg(REG_MCSM2,VAL_MCSM2);
  WriteReg(REG_MCSM1,VAL_MCSM1);
  WriteReg(REG_MCSM0,VAL_MCSM0);
  WriteReg(REG_FOCCFG,VAL_FOCCFG);
  WriteReg(REG_BSCFG,VAL_BSCFG);
  WriteReg(REG_AGCCTRL2,VAL_AGCCTRL2);
  WriteReg(REG_AGCCTRL1,VAL_AGCCTRL1);
  WriteReg(REG_AGCCTRL0,VAL_AGCCTRL0);
  WriteReg(REG_WOREVT1,VAL_WOREVT1);
  WriteReg(REG_WOREVT0,VAL_WOREVT0);
  WriteReg(REG_WORCTRL,VAL_WORCTRL);
  WriteReg(REG_FREND1,VAL_FREND1);
  WriteReg(REG_FREND0,VAL_FREND0);
  WriteReg(REG_FSCAL3,VAL_FSCAL3);
  WriteReg(REG_FSCAL2,VAL_FSCAL2);
  WriteReg(REG_FSCAL1,VAL_FSCAL1);
  WriteReg(REG_FSCAL0,VAL_FSCAL0);
  WriteReg(REG_RCCTRL1,VAL_RCCTRL1);
  WriteReg(REG_RCCTRL0,VAL_RCCTRL0);
  WriteReg(REG_FSTEST,VAL_FSTEST);
  WriteReg(REG_TEST2,VAL_TEST2);
  WriteReg(REG_TEST1,VAL_TEST1);
  WriteReg(REG_TEST0,VAL_TEST0);
  WriteReg(REG_DAFUQ,VAL_DAFUQ);
}
