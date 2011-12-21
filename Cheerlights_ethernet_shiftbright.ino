/*

 CheerLights!
 
 CheerLights Channel --> Arduino Ethernet --> ShiftBrite(s)
 
 This sketch subscribes to the CheerLights ThingSpeak Channel and relays the
 last command from the Internet to a chain of ShiftBrite lights. This
 sketch uses DHCP to obtain an IP address automatically and requires Arduino 1.0.
 
 Arduino Requirements:
 
   * Arduino Ethernet (or Arduino + Ethernet Shield)
   * Arduino 1.0 IDE
   
 Network Requirements:

   * Ethernet port on Router    
   * DHCP enabled on Router
   * Unique MAC Address for Arduino

 Getting a Globally Unique MAC Address from ThingSpeak:
 
   * Sign Up for New User Account - https://www.thingspeak.com/users/new
   * Register your Arduino by selecting Devices, Add New Device
   * Once the Arduino is registered, click Generate Unique MAC Address
   * Enter the new MAC Address in this sketch under "Local Network Settings"
 
 Created: Novemeber 29, 2011 by Hans Scharler - http://www.iamshadowlord.com
 Beat into submission to run Shiftbrites on Dec 20, 2011 by Ben Konosky http://www.konosky.org
  
 Additional Credits:
 http://www.digitalmisery.com
 http://www.deepdarc.com
 http://www.iamshadowlord.com
 http://rasterweb.net
 
 To join the CheerLights project, visit http://www.cheerlights.com
 
*/

#include <SPI.h>
#include <Ethernet.h>


// ShiftBrite settings.
#define numberSB 1 //Number of ShiftBrites in the chain
#define datapin 7 //DI
#define latchpin 5 //LI
#define enablepin 6 //EI
#define clockpin 8 //CI
#define redlevel 120 // Power level for Red 0-127
#define bluelevel 100 // Power level for Blue 0-127
#define greenlevel 100 // Power level for Green 0-127

unsigned long SB_CommandPacket;
int SB_CommandMode;
int SB_BlueCommand;
int SB_RedCommand;
int SB_GreenCommand;

//LED Colors setup
const int RED[] = {1000, 0, 0}; 
const int ORANGE[] = {1000, 500, 0}; 
const int YELLOW[] = {900, 1000, 0}; 
const int GREEN[] = {0, 1000, 0}; 
const int BLUE[] = {0, 0, 1000}; 
const int CYAN[] = {0, 1000, 700}; 
const int MAGENTA[] = {700, 0, 1000}; 
const int WHITE[] = {900, 900, 900}; 
const int WARMWHITE[] = {900, 600, 700};
const int BLACK[] = {0, 0, 0}; //?
const int PURPLE[] = {300, 200, 1000}; 

// Fade settings
#define stepdivisor 2 // Fade with fewer steps
#define stepdelay 1 // Increase delay between steps

static uint16_t c;


// Local Network Settings
byte mac[] = { 0xD4, 0x28, 0xB2, 0xFF, 0x4C, 0x84 }; // Must be unique on local network

// ThingSpeak Settings
// I had to comment out and put directly into code most of this due to RAM limits.
//char thingSpeakAddress[] = "api.thingspeak.com";
//String thingSpeakChannel = "1417";                // Channel number of the CheerLights Channel
//String thingSpeakField = "1";                     // Field number of the CheerLights commands 
#define thingSpeakInterval 16000      // Time interval in milliseconds to get data from ThingSpeak (number of seconds * 1000 = interval)

// Variable Setup
long lastConnectionTime = 0; 
int lastCommand[] = {0, 0, 0};
String lastCommandString = "black";
boolean lastConnected = false;
int failedCounter = 0;

// Initialize Arduino Ethernet Client
EthernetClient client;

void SB_SendPacket() {

   SB_CommandPacket = SB_CommandMode & B11;
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_BlueCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_RedCommand & 1023);
   SB_CommandPacket = (SB_CommandPacket << 10)  | (SB_GreenCommand & 1023);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 24);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 16);
   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket >> 8);

   shiftOut(datapin, clockpin, MSBFIRST, SB_CommandPacket);


   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,HIGH); // latch data into registers
   delay(1); // adjustment may be necessary depending on chain length
   digitalWrite(latchpin,LOW);
}


void setColor(int* color) {
   for (int j = 0; j < numberSB; j++) {
     SB_CommandMode = B01; // Write to current control registers
     SB_RedCommand = redlevel; 
     SB_GreenCommand = greenlevel; 
     SB_BlueCommand = bluelevel; 
     SB_SendPacket();
   }
   for (int j = 0; j< numberSB; j++) {
     SB_CommandMode = B00;
     SB_RedCommand =  color[0];
     SB_GreenCommand =  color[1];
     SB_BlueCommand =  color[2];
     SB_SendPacket();
   }
}
  

void setColor(const int* color ) { 
  int tempInt[] = {color[0], color[1], color[2]}; 
   setColor(tempInt);
}



void setup() {
  delay(100);
  // Setup Serial
  Serial.begin(9600);
  delay(100);
  
  Serial.flush();
  delay(100);
   pinMode(datapin, OUTPUT);
   pinMode(latchpin, OUTPUT);
   pinMode(enablepin, OUTPUT);
   pinMode(clockpin, OUTPUT);

   digitalWrite(latchpin, LOW);
   digitalWrite(enablepin, LOW);
   setColor(BLACK);
  delay(100);
  // Start Ethernet on Arduino
  startEthernet();
}

void loop() {
   
  // Process CheerLights Commands
  if(client.available() > 0)
  {  
    delay(100); 
    Serial.println(client.available());
    String response;
    char charIn;
    do {
        charIn = client.read(); // read a char from the buffer
        response += charIn; // append that char to the string response
      } while (client.available() > 0);
        Serial.println(response.length());
        Serial.println(response); 

    // Send matching commands to the GE-35 Color Effect Lights
    if (response.indexOf("white") > 0)
    {  
        lastCommandString = "white";
        fadeToColor(lastCommand,WHITE,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = WHITE[i];
        }
    }
    else if (response.indexOf("black") > 0)
    {  
        lastCommandString = "black";
        fadeToColor(lastCommand,BLACK,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = BLACK[i];
        }
    }
    else if (response.indexOf("red") > 0)
    {  
        lastCommandString = "red";
        fadeToColor(lastCommand,RED,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = RED[i];
        }
    }
    else if (response.indexOf("green") > 0)
    {  
        lastCommandString = "green";
        fadeToColor(lastCommand,GREEN,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = GREEN[i];
        }
    }
    else if (response.indexOf("blue") > 0)
    {  
        lastCommandString = "blue";  
        fadeToColor(lastCommand,BLUE,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = BLUE[i];
        }
    }
    else if (response.indexOf("cyan") > 0)
    {  
        lastCommandString = "cyan";
        fadeToColor(lastCommand,CYAN,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = CYAN[i];
        }
    }
    else if (response.indexOf("magenta") > 0)
    {  
        lastCommandString = "magenta";
        fadeToColor(lastCommand,MAGENTA,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = MAGENTA[i];
        }
    }
    else if (response.indexOf("yellow") > 0)
    {  
        lastCommandString = "yellow";
        fadeToColor(lastCommand,YELLOW,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = YELLOW[i];
        }
    }
    else if (response.indexOf("purple") > 0)
    {  
        lastCommandString = "purple";
        fadeToColor(lastCommand,PURPLE,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = PURPLE[i];
        }
    }
    else if (response.indexOf("orange") > 0)
    {  
        lastCommandString = "orange";
        fadeToColor(lastCommand,ORANGE,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = ORANGE[i];
        }
    }
    else if (response.indexOf("warmwhite") > 0)
    {  
        lastCommandString = "warmwhite";
        fadeToColor(lastCommand,WARMWHITE,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = WARMWHITE[i];
        }
    }
    else if (response.indexOf("black") > 0)
    {  
        lastCommandString = "black";
        fadeToColor(lastCommand,BLACK,stepdelay);
        for (int i = 0; i < 3; i++) {
          lastCommand[i] = BLACK[i];
        }
    }
    else
    {
        lastCommandString = "(no match)";  
    }
    
    // Echo command
    delay(200);
    Serial.print("CheerLight Command Received: ");
    Serial.println(lastCommandString);
    delay(200); 
    
  }
  
    // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    Serial.println("...disconnected");
    
    client.stop();
  }
  
  // Subscribe to ThingSpeak Channel and Field
  if(!client.connected() && (millis() - lastConnectionTime > thingSpeakInterval))
  {
    subscribeToThingSpeak();
  }
  
  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 3 ) {startEthernet();}
  
  lastConnected = client.connected();
delay(100);  
} // End loop

void subscribeToThingSpeak()
{
  if (client.connect("api.thingspeak.com", 80))
  {         
    Serial.println("Connecting to ThingSpeak...");
      
    failedCounter = 0;
    Serial.println("Sending Request");
    client.println("GET /channels/1417/field/1/last.txt HTTP/1.0");
    client.println();
    
    lastConnectionTime = millis();
  }
  else
  {
    failedCounter++;
    
    Serial.println("Connection to ThingSpeak Failed ("+String(failedCounter, DEC)+")");   
    Serial.println();
    
    lastConnectionTime = millis(); 
  }
}

void startEthernet()
{
  
  client.stop();

  Serial.println("Connecting Arduino to network...");
  Serial.println();  

  delay(1000);
  
  // Connect to network amd obtain an IP address using DHCP
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("DHCP Failed, reset Arduino to try again");
    Serial.println();
  }
  else
  {
    Serial.println("Arduino connected to network using DHCP");
    Serial.println();
  }
  
  delay(1000);
}

void fadeToNumColor(int* startColor, int* endColor, int fadeSpeed) {  
  int changeRed = endColor[0] - startColor[0];                             //the difference in the two colors for the red channel  
  int changeGreen = endColor[1] - startColor[1];                           //the difference in the two colors for the green channel   
  int changeBlue = endColor[2] - startColor[2];                            //the difference in the two colors for the blue channel  
  int steps = (max(abs(changeRed),max(abs(changeGreen), abs(changeBlue)))/stepdivisor);  //make the number of change steps the maximum channel change    
//  Serial.println(steps);
  for(long i = 0 ; i < steps; i++) {                                        //iterate for the channel with the maximum change   
    int newRed = startColor[0] + (i * changeRed / steps);                 //the newRed intensity dependant on the start intensity and the change determined above   
    int newGreen = startColor[1] + (i * changeGreen / steps);             //the newGreen intensity   
    int newBlue = startColor[2] + (i * changeBlue / steps);               //the newBlue intensity   
    
    int newColor[] = {newRed, newGreen, newBlue};                         //Define an RGB color array for the new color   
    setColor(newColor);                                               //Set the LED to the calculated value   
    delay(fadeSpeed);                                                      //Delay fadeSpeed milliseconds before going on to the next color  
  }  
  setColor(endColor);                                                 //The LED should be at the endColor but set to endColor to avoid rounding errors
}



void fadeToColor(const int* startColor, const int* endColor, int fadeSpeed) {  
  int tempByte1[] = {startColor[0], startColor[1], startColor[2]};   
  int tempByte2[] = {endColor[0], endColor[1], endColor[2]};   
  fadeToNumColor(tempByte1, tempByte2, fadeSpeed);
}

