//Hi there! This is the code for the HeatSync Labs Robot Fleet!
//Read through these comments to understand what this code does
//-------------------------------------------------------------
//This code must emit "Greetings from HeatSync Labs" upon bootup
//http://www.heatsynclabs.org

#include <string.h>
#include <NeoSWSerial.h>
#include <Adafruit_NeoPixel.h>
//#include <SoftwareSerial.h> //outdated, and replaced by NeoSWSerial!

//We will use the IRL Remote library to recieve IR remote signals
#include "IRLremote.h"

//We will use the following pins for the motors
//These need to be ~ pins (analog capabale)
#define L_FWD_PIN 3
#define L_REV_PIN 5
#define R_FWD_PIN 6
#define R_REV_PIN 9

// Our IR Reciever will be plugged into this pin
#define pinIR 2

//Uncomment below to output everything over the serial port. For debug only! this will induce a LOT of lag!
#define DEBUGSERIAL

//IR remote object
CNec IRLremote;

//pin to use for serial port to on-robot sound controllers
#define SNDSERIAL_PIN 8

//Some IR remotes use repeat codes if a button is held down. We will use these to track
int lastcommand = 0;
int lastaddress = 0;

//Watchdog Timer to stop robot if no commands come in
unsigned long LastCmdTime = millis();

//Neopixel Setup
//Neopixel Pin
#define PIXEL_PIN 12
//Neo Pixel Object (set number of pixels here)
Adafruit_NeoPixel LEDstrip = Adafruit_NeoPixel(2,  PIXEL_PIN, NEO_GRB + NEO_KHZ800);

NeoSWSerial sndSerial(SNDSERIAL_PIN, SNDSERIAL_PIN); // RX, TX

//This is run ONCE when you power up
void setup()
{
  // initialize serial port:
  Serial.begin(2400);
  sndSerial.begin(9600); //software

  // Start looking for IR codes:
  if (!IRLremote.begin(pinIR))
    Serial.println(F("Sorry, this pin can't support IR recieving :("));

  //Initialize the timer
  unsigned long tMillis = millis();

  //Initialize the neo pixels and display a dim white so we know they are working
  LEDstrip.begin();
  LEDstrip.setPixelColor(0, 8, 8, 8);
  LEDstrip.setPixelColor(1, 8, 8, 8);
  LEDstrip.show();

  //Pin A3 can be used to power the IR reciever as it only draws 1.5mA (20mA max on this pin!)
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH); //provide power to the IR reciever

  //Say hello to everyone!
  Serial.println(F("Greetings from HeatSync Labs!"));
}

//This function will read serial port data. It will look for a string of characters that ends with a newline or a semicolon
//It will return everything before the semicolon

bool getSerialCommand(String &readString, int timeoutVal = 10) //Note that readString is passed by reference!
{ //this function will always hang around for timeoutVal milliseconds.
  //Consider re-writing this code if we need to process other inputs doing this wait period
  char numChars = 0;
  unsigned long timeout = millis();
  while ((millis() - timeout) < timeoutVal) //Wait for a little while for the next character
  {
    if (Serial.available() > 0) //We have data waiting for us!
    {
      char c = Serial.read();  //Grab it! This gets one byte from serial buffer
      numChars++; //keep track of how long our command is
      if (c == '\n' || c == '\r' || c == ';') //If we get a newline, or a semicolon, process this command
      {
        return true; //note that the command will NOT have the ';' character at the end!
      }
      if (numChars++ > 32) //We will never see a command this long! Abort abort!
        return false;
      readString += c; //Add the character to the end of the command and keep listening
    }
  }
  return false; //timed out, no command here
}

//This function processes the commands. These commands can either come into from the serial power, can be issued from IR codes
//or can be called manually (in case of automatic modes)
void executeCommand(String command)
{
  int bVal = 0;
  //The protocol looks like FWD=50; REV=30; RAW=12,-150;

  //look inside these if statements for the commands (FWD=, REV=, RAW=, STP, etc)
  if (command.startsWith("SND="))
  {
    command.remove(0, 4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
#ifdef DEBUGSERIAL
    Serial.print("SND=");
    Serial.println(bVal, DEC);
#endif
  sndSerial.write(bVal+47);
  }
  else if (command.startsWith("FWD=") )
  {
    //Turn motors forward at a given speed.
    command.remove(0, 4); //remove the command, the leftovers should be the value
    bVal = command.toInt();

#ifdef DEBUGSERIAL
    Serial.print("FWD=");
    Serial.println(bVal, DEC);
#endif

    analogWrite(L_REV_PIN, 0);
    analogWrite(R_REV_PIN, 0);
    analogWrite(L_FWD_PIN, bVal);
    analogWrite(R_FWD_PIN, bVal);
  }
  else if (command.startsWith("REV=") )
  {
    //Turn motors backward at a given speed.
    command.remove(0, 4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
#ifdef DEBUGSERIAL
    Serial.print("REV=");
    Serial.println(bVal, DEC);
#endif

    analogWrite(L_FWD_PIN, 0);
    analogWrite(R_FWD_PIN, 0);
    analogWrite(L_REV_PIN, bVal);
    analogWrite(R_REV_PIN, bVal);
  }
  else if (command.startsWith("LFT=") )
  {
    //Turn left at a given speed.
    command.remove(0, 4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
#ifdef DEBUGSERIAL
    Serial.print("LFT=");
    Serial.println(bVal, DEC);
#endif

    //Write zeros first to make sure we don't accidentally short an H-bridge leg
    analogWrite(L_FWD_PIN, 0);
    analogWrite(R_REV_PIN, 0);
    analogWrite(L_REV_PIN, bVal);
    analogWrite(R_FWD_PIN, bVal);

  }
  else if (command.startsWith("RGT=") )
  {
    //Turn right at a given speed.
    command.remove(0, 4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
#ifdef DEBUGSERIAL Serial.print("RGT=");
    Serial.println(bVal, DEC);
#endif

    //Write zeros first to make sure we don't accidentally short an H-bridge leg
    analogWrite(L_REV_PIN, 0);
    analogWrite(R_FWD_PIN, 0);
    analogWrite(L_FWD_PIN, bVal);
    analogWrite(R_REV_PIN, bVal);
  }
  else if (command.startsWith("RAW=") )
  {
    int bVal2 = 0, idx = 0;
    String subString;
    //Control F/B and L/R at the same time from an analog driver. Range is from -255 to +255 with two values
    //Syntax is Raw=127,-100;
    //command.remove(0,4); //remove the command, the leftovers should be the value
    idx = command.indexOf(',');  //finds location of first
    if (idx <= 0 || idx == (command.length() - 1))
      return; //Bail, no comma or no second number

    subString = command.substring(4, idx);
    bVal = subString.toInt();   //captures first data String
    subString = command.substring(idx + 1, command.length());
    bVal2 = subString.toInt();   //captures first data String

#ifdef DEBUGSERIAL
    Serial.print("RAW=");
    Serial.print(bVal, DEC);
    Serial.print(',');
    Serial.print(bVal2, DEC);
    Serial.print(':');
    Serial.print(command.length(), HEX);
    Serial.print(',');
    Serial.println(idx, HEX);
#endif

    if (bVal >= 0 && bVal2 >= 0) //we're moving forward and right
    {
#ifdef DEBUGSERIAL
      Serial.println("FWR");
#endif
      if (bVal - bVal2 >= 0)
      {
        analogWrite(R_REV_PIN, 0);
        analogWrite(R_FWD_PIN, constrain(bVal - bVal2, 0, 255));
      }
      else
      {
        analogWrite(R_FWD_PIN, 0);
        analogWrite(R_REV_PIN, constrain(bVal2 - bVal, 0, 255));
      }
      analogWrite(L_REV_PIN, 0);
      analogWrite(L_FWD_PIN, constrain(bVal + bVal2, 0, 255));
    }
    else if (bVal <= 0 && bVal2 >= 0) //we're moving backward and right
    {
      bVal *= -1;
#ifdef DEBUGSERIAL
      Serial.println("RVR");
#endif
      if (bVal - bVal2 >= 0)
      {
        analogWrite(R_FWD_PIN, 0);
        analogWrite(R_REV_PIN, constrain(bVal - bVal2, 0, 255));
      }
      else
      {
        analogWrite(R_REV_PIN, 0);
        analogWrite(R_FWD_PIN, constrain(bVal2 - bVal, 0, 255));
      }
      analogWrite(L_FWD_PIN, 0);
      analogWrite(L_REV_PIN, constrain(bVal + bVal2, 0, 255));
    }
    else if (bVal >= 0 && bVal2 <= 0) //we're moving forward and left
    {
      bVal2 *= -1;
#ifdef DEBUGSERIAL
      Serial.println("FWL");
#endif
      if (bVal - bVal2 >= 0)
      {
        analogWrite(L_REV_PIN, 0);
        analogWrite(L_FWD_PIN, constrain(bVal - bVal2, 0, 255));
      }
      else
      {
        analogWrite(L_FWD_PIN, 0);
        analogWrite(L_REV_PIN, constrain(bVal2 - bVal, 0, 255));
      }
      analogWrite(R_REV_PIN, 0);
      analogWrite(R_FWD_PIN, constrain(bVal + bVal2, 0, 255));
    }
    else if (bVal <= 0 && bVal2 <= 0) //we're moving backward and left
    {
      bVal *= -1;
      bVal2 *= -1;
#ifdef DEBUGSERIAL
      Serial.println("RVL");
#endif
      if (bVal - bVal2 >= 0)
      {
        analogWrite(L_FWD_PIN, 0);
        analogWrite(L_REV_PIN, constrain(bVal - bVal2, 0, 255));
      }
      else
      {
        analogWrite(L_REV_PIN, 0);
        analogWrite(L_FWD_PIN, constrain(bVal2 - bVal, 0, 255));
      }
      analogWrite(R_FWD_PIN, 0);
      analogWrite(R_REV_PIN, constrain(bVal + bVal2, 0, 255));
    }
    //analogWrite(L_REV_PIN, 0);
    //analogWrite(R_FWD_PIN, 0);
    //analogWrite(L_FWD_PIN, bVal);
    //analogWrite(R_REV_PIN, bVal);
  }
  else if (command.startsWith("STP"))
  {
    //STOP!
    analogWrite(L_FWD_PIN, 0);
    analogWrite(L_REV_PIN, 0);
    analogWrite(R_FWD_PIN, 0);
    analogWrite(R_REV_PIN, 0);
  }
  
  else if (command.startsWith("HLO"))
  {
    //Head Lights On
    if (LEDstrip.getPixelColor(1) < 0xFFFFFF)
    {
      //If they are off, turn them on
      LEDstrip.setPixelColor(0, 255, 255, 255);
      LEDstrip.setPixelColor(1, 255, 255, 255);
      LEDstrip.show();
    }
    else
    {
      //If they are on, turn them off
      LEDstrip.setPixelColor(0, 0, 0, 0);
      LEDstrip.setPixelColor(1, 0, 0, 0);
      LEDstrip.show();
    }
  }
}

//This command decodes IR remotes into commands that you choose
bool decodeIRCommand(String &command)
{
  bool validCmd = true; //Initial guess -- check later on
  //String command;
  auto data = IRLremote.read(); //Get the IR address and command
  if (data.address == 0xFFFF && data.command == 0x0) //This is a repeat code, repeat the last action
  {
    //Inejct the previous command into the command decoder
    //"Do the thing you just did again"
    data.command = lastcommand;
    data.address = lastaddress;
  }

  if (data.address == 0x6B86) //This is an random Chinese projector remote
  {
    switch (data.command)
    {
      case 0x2: command = "FWD=128"; break;
      case 0x6: command = "REV=128"; break;
      case 0x4: command = "LFT=128"; break;
      case 0x7: command = "RGT=128"; break;
      case 0x1: command = "HLO"; break;
      default: Serial.println(data.command, HEX); validCmd = false; //This button isn't mapped yet, print the code!
    }
  }
  else if (data.address == 0xFF00) //This is an RTLSDR remote
  {
    switch (data.command)
    {
      case 0x5: command = "FWD=128"; break;
      case 0x2: command = "REV=128"; break;
      case 0xA: command = "LFT=128"; break;
      case 0x1E: command = "RGT=128"; break;
      case 0x40: command = "STP"; break;
      case 0x16: command = "HLO"; break;
      default: Serial.println(data.command, HEX); validCmd = false; //This button isn't mapped yet, print the code!
    }
  }
  else
  {
    //Don't know what remote this is. Print the codes so we can tell how to use it!
    Serial.print(data.address, HEX);
    Serial.print(",");
    Serial.println(data.command, HEX);
    validCmd = false; //no valid command :(
  }

  //Did we get a valid command?
  if (validCmd == true)
  {
    lastcommand = data.command; //Set up the variables needed for a repeat code next loop
    lastaddress = data.address;
    return true;
  }
  else
    return false;
}

void loop()
{
  String readString = ""; //the incoming command will be put here.

  //is serial data waiting?
  if (Serial.available() > 0)
  {
    if (getSerialCommand(readString, 75) == true) //did we get good data?
    {
      //#ifdef DEBUGSERIAL Serial.println(readString); #endif
      LastCmdTime = millis(); //reset the timeout timer
      executeCommand(readString); //this can also be used to feed in commands manually! executeCommand("FWD"); for example!
    }
  }

  //is IR remote data waiting?
  if (IRLremote.available())
  {
    // Get the new data from the remote
    if (decodeIRCommand(readString) == true) //did we get good data?
    {
      //#ifdef DEBUGSERIAL Serial.println(readString); #endif
      LastCmdTime = millis(); //reset the timeout timer
      executeCommand(readString); //this can also be used to feed in commands manually! executeCommand("FWD"); for example!
    }
  }

  //Has the dog been placated recently?
  if ( millis() - LastCmdTime > 200) //200mS timeout -- stop if its been more than 200mS
  {
#ifdef DEBUGSERIAL
    Serial.println("STP");
#endif
    executeCommand("STP");
    LastCmdTime = millis(); //Call this a command, and avoid hammering the stop command
  }
}
