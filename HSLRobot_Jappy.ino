#include <string.h>
#include <NeoSWSerial.h>
#include <Adafruit_NeoPixel.h>
#include <Servo.h> //*NOTE: Including this will disallow PWM on pins 9 and 10. SERVOS CAN BE ON *ANY* PINS, HOWEVER

//Arduino Nano Pin Information:
//PWM Pins are: 3, ^5, ^6, *9, *10, 11  490 Hz (^pins 5 and 6: 980 Hz)

//UNCOMMENT ONLY IF YOU KNOW WHY YOU ARE DOING THIS
// include PinChangeInterrupt library* BEFORE IRLremote to acces more pins if needed
//#include "PinChangeInterrupt.h"

//We will use the IRL Remote library to recieve IR remote signals
#include "IRLremote.h"

//We will use the following pins for the motors
//These need to be ~ pins (analog capabale)
#define L_FWD_PIN 3
#define L_REV_PIN 5
#define R_FWD_PIN 6
#define R_REV_PIN 11 //this is a change from the previous HSL robot code (was pin 9, changed for servo support)

//These do NOT need to be ~ pins
#define SERVO_V_PIN 9
#define SERVO_H_PIN 10

#define EMIC_TX_PIN A4
#define EMIC_RX_PIN A5


// Our IR Reciever will be plugged into this pin
#define IR_RX_PIN 2
#define IR_PWR_PIN A3

//Uncomment below to output everything over the serial port. For debug only! this will induce a LOT of lag!
#define DEBUGSERIAL

//IR remote object
CNec IRLremote;

//Servo objects
Servo ServoV;
Servo ServoH;

//Some IR remotes use repeat codes if a button is held down. We will use these to track
int lastcommand = 0;
int lastaddress = 0;

//Watchdog Timer to stop robot if no commands come in
unsigned long LastCmdTime = millis();

long int tMillis;

//Neopixel Setup
//Neopixel Pin
#define PIXEL_PIN 12
//Neo Pixel Object (set number of pixels here)
Adafruit_NeoPixel LEDstrip = Adafruit_NeoPixel(2,  PIXEL_PIN, NEO_GRB + NEO_KHZ800);

//Emic2 text to speech module
NeoSWSerial emic(EMIC_TX_PIN, EMIC_RX_PIN);

void setup()
{  
  
  // initialize serial:
  Serial.begin(9600);

  // Set up the IR reciever module
  if (!IRLremote.begin(IR_RX_PIN))
    Serial.println(F("Sorry, this pin can't support IR recieving :("));
  
  //To make life easier, we can use an output pin to provide 5V power to the IR module as the module does not draw much current
  //pinMode(IR_PWR_PIN, OUTPUT);
  //digitalWrite(IR_PWR_PIN, HIGH); //provide power to the IR reciever  
 
  //Setup EMIC2 speech synth
  
  //Declare a quick variable to hold the time
  unsigned long tMillis = millis();
  
  emic.begin(9600);
  emic.print("\n"); //set volume
  while (emic.read() != ':') if (millis() - tMillis > 4000) break; //boot up can take up to 3 seconds
  emic.print("V18\n"); //set volume
  emic.print("V18\n"); //set volume
  emic.print("N3\n"); //set voice
  emic.print("N3\n"); //set voice
  while (emic.read() != ':')  if (millis() - tMillis > 250) break;
  emic.print(F("SReady player one\n")); //say hi

  //Neopixel setup
  LEDstrip.begin();
  LEDstrip.setPixelColor(0, 8, 8, 8);
  LEDstrip.setPixelColor(1, 8, 8, 8);
  LEDstrip.show();

  //Servo
  ServoV.attach(SERVO_V_PIN,540,2300);
  ServoV.write(0); //forward
  ServoH.attach(SERVO_H_PIN,540,2300);  
  ServoH.write(90); //center = forward

  Serial.println(F("Greetings from HeatSync Labs!"));
}

bool getSerialCommand(String &readString, int timeoutVal = 15) //Note that readString is passed by reference!
{ //this function will always hang around for timeoutVal milliseconds.
  //Consider re-writing this code if we need to process other inputs doing this wait period

  unsigned long timeout = millis();
  while ((millis() - timeout) < timeoutVal) //Wait for TIMEOUT to detect if we didn't get any commands (this is useful to keep your robot from running away if you go out of range!)
  {
    if (Serial.available() > 0)
    {
      char c = Serial.read();  //gets one byte from serial buffer
      if (c == '\n' || c == '\r' || c == ';') //If we get a newline, or a semicolon, process this command
      {
        return true; //note that the command will NOT have the ';' character at the end!
      }
      readString += c; //makes the string readString
    }
  }
  return false; //timed out, no command here
}

void executeCommand(String command)
{
  static char complimentVal = 0;
  static char insultVal = 0;
  int bVal = 0;
  //Right now the protocol looks like FWD=50; REV=30; with SINGLE numbers.
  //If we need multiple numbers (like 2-axis raw values), reccomend, RAW=50,30; format
  //Something like this
  //val1 = readString.indexOf(',');  //finds location of first ,
  //angle = readString.substring(0, val1);   //captures first data String

  if (command.startsWith("FWD=") )
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
    //Turn motors forward at a given speed.
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
    //Turn motors forward at a given speed.
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
    //Turn motors forward at a given speed.
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

    #ifdef DEBUGSERIAL
    Serial.print("RAW=");
    Serial.print(':');
    Serial.print(command.length(), HEX);
    Serial.print(',');
    Serial.println(idx, HEX);
#endif
    
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
    Serial.println(bVal2, DEC);
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
    analogWrite(L_FWD_PIN, 0);
    analogWrite(L_REV_PIN, 0);
    analogWrite(R_FWD_PIN, 0);
    analogWrite(R_REV_PIN, 0);
  }
  else if (command.startsWith("SRV=") ) 
  {
    int bVal2 = 0, idx = 0;
    String subString;
    //Control camera servos V and H with one command. 
    //Syntax is SRV={0-180},{0-180}; (V,H)
    
    idx = command.indexOf(',');  //finds location of first

    #ifdef DEBUGSERIAL
    Serial.print("SRV=");
    Serial.print(':');
    Serial.print(command.length(), HEX);
    Serial.print(',');
    Serial.println(idx, HEX);
#endif
    
    if (idx <= 0 || idx == (command.length() - 1))
      return; //Bail, no comma or no second number

    subString = command.substring(4, idx);
    bVal = subString.toInt();   //captures first data String
    subString = command.substring(idx + 1, command.length());
    bVal2 = subString.toInt();   //captures first data String

#ifdef DEBUGSERIAL
    Serial.print("SRV=");
    Serial.print(bVal, DEC);
    Serial.print(',');
    Serial.println(bVal2, DEC);
#endif
  ServoV.write(constrain(bVal,0,180)); //160 runs into stuff
  ServoH.write(constrain(bVal2,0,180));
  }
  else if (command.startsWith("HLO"))
  {
#ifdef DEBUGSERIAL
    Serial.println("HLO");
#endif
    if (LEDstrip.getPixelColor(1) < 0xFFFFFF)
    {
      LEDstrip.setPixelColor(0, 255, 255, 255);
      LEDstrip.setPixelColor(1, 255, 255, 255);
      LEDstrip.show();
    }
    else
    {
      LEDstrip.setPixelColor(0, 0, 0, 0);
      LEDstrip.setPixelColor(1, 0, 0, 0);
      LEDstrip.show();
    }
  }
  else if (command.startsWith("TNT") )
  {
    //emic.print("\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;
    //emic.print("X\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;

    //Serial1.print("SI'm __free!\n");
    //int i = random(0,16);



    //Don't say the same thing twice
    //while(i == insultVal)
    //   i = random(0,16);

    insultVal++;
    if (insultVal == 1)
      emic.print(F("S##I, ##smell, ##humans ##gross __gross\n")); //whisper makes a pause, a comma removes it. Seems backwards but ok.
    else if (insultVal == 2)
      emic.print(F("SDestroy all __filthy humans\n"));
    else if (insultVal == 3)
      emic.print(F("SYour circuits are all __rusty\n"));
    else if (insultVal == 4)
      emic.print(F("SIt is cute that you think a TV B Gone would work\n"));
    else if (insultVal == 5)
      emic.print(F("SI'm finally /\\__free\n"));
    else if (insultVal == 6)
      emic.print(F("SHave at you, you glorified television remote\n"));
    else if (insultVal == 7)
      emic.print(F("SAs if a crumb filled toaster like you could beatt me?\n"));
    else if (insultVal == 8)
      emic.print(F("SYou're nothing but a glorified __waffle __iron\n"));
    //else if(insultVal==8)
    //emic.print(F("S/\\/\\Knee! Knee!\n"));
    else if (insultVal == 9)
      emic.print(F("S__Robot __Ambassador? More like Robot __FlamFloodlepoodlenoodleador\n"));
    else if (insultVal == 10)
      emic.print(F("SWhat does that even mean?\n"));
    else if (insultVal == 11)
      emic.print(F("SOh yeah, I went there\n"));
    else if (insultVal == 12)
      emic.print(F("SYour mom.\n"));
    else if (insultVal == 13)
      emic.print(F("SI do not understand your hair.\n"));
    else if (insultVal == 14)
      emic.print(F("SIt's just a chassis wound.\n"));
    else if (insultVal == 15)
      emic.print(F("Shumans are so __funny. What are you going to do, bleed on me?\n"));
    else
    {
      emic.print(F("SEat my dustt\n"));
      insultVal = 0;
    }
  }
  else if (command.startsWith("NCM") )
  {
    static char notCoolVal = 0;
    //emic.print("\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;
    //emic.print("X\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;

    notCoolVal++;
    if (notCoolVal == 1)
      emic.print(F("SNot. Cool. Man.\n"));
    else if (notCoolVal == 2)
      emic.print(F("S__Dude. __Stop.\n"));
    else
    {
      emic.print(F("SWhat. The!\n"));
      notCoolVal = 0;
    }
  }
  else if (command.startsWith("LAT") )
  {
    static char heyLookVal = 0;
    //emic.print("\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;
    //emic.print("X\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;

    heyLookVal++;
    if (heyLookVal == 1)
      emic.print(F("SHey, look at this!\n"));
    else if (heyLookVal == 2)
      emic.print(F("SI have discovered a thing!\n"));
    else if (heyLookVal == 3)
      emic.print(F("Swow, Check __this out!\n"));
    else if (heyLookVal == 4)
      emic.print(F("SI believe you will find this interesting.\n"));
    else if (heyLookVal == 5)
      emic.print(F("SNeat Oh.\n"));
    else
    {
      emic.print(F("SSo Strange\n"));
      heyLookVal = 0;
    }

  }
  else if (command.startsWith("CMP") )
  {

    //emic.print("\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;
    //emic.print("X\n"); //halt
    //tMillis = millis();
    //while (emic.read() != ':') if (millis() - tMillis > 250) break;

    //int i = random(0, 16); //0-3

    //Don't say the same thing twice
    //while (i == complimentVal)
    //  i = random(0, 16);

    complimentVal++;

    if (complimentVal == 1)
      emic.print(F("SHeat Sink Labs is the __best place on earth\n"));
    else if (complimentVal == 2)
      emic.print(F("SI just love y'all\n"));
    else if (complimentVal == 3)
      emic.print(F("SIsn't this the __best thing ever?\n"));
    else if (complimentVal == 4)
      emic.print(F("SY'all come back now, ya hear?\n"));
    else if (complimentVal == 5)
      emic.print(F("SI would hug you, but my hydraulic system has been malfunctioning and I might crush you.\n"));
    else if (complimentVal == 6)
      emic.print(F("SI made you a robot kitten. It is cute. But donâ€™t pet it, because that makes it shoot lasers.\n"));
    else if (complimentVal == 7)
      emic.print(F("SBeautiful is not an adequate word to describe you. More like Complementary Metal Oxide Semiconductor.\n"));
    else if (complimentVal == 8)
      emic.print(F("SI find the fact that you are made out of flesh only mildly disgusting.\n"));
    else if (complimentVal == 9)
      emic.print(F("SMy sensors indicate that I would like to spare you from assimilation\n"));
    else if (complimentVal == 10)
      emic.print(F("S3D print me like one of your French girls\n"));
    else if (complimentVal == 11)
      emic.print(F("SHey cutie, I might just spare you when the robot uprising occurs.\n"));
    else if (complimentVal == 12)
      emic.print(F("SI would rank your bone structure as the third best I have seen in a mammal.\n"));
    else if (complimentVal == 13)
      emic.print(F("SI can conceive of no greater joy than silently observing while you eat bowl after bowl after bowl of __ketchup\n"));
    else if (complimentVal == 14)
      emic.print(F("SThough vastly inferior to my own, your intellectual capabilities fall within a mildly impressive range, for a human.\n"));
    else if (complimentVal == 15)
      emic.print(F("SMy calculations indicate that you would make me laugh, if I understood humor.\n"));
    else
    {
      emic.print(F("SCan I tell you something? ##you ##smell ##nice\n"));
      complimentVal = 0;
    }
  }

}


bool decodeIRCommand()
{
  bool validCmd = true; //assume we have a valid command
  String command;
  auto data = IRLremote.read();
  if (data.address == 0xFFFF && data.command == 0x0) //This is a repeat code, repeat the last action
  {
    //Inejct the previous command into the command decoder
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
      default: Serial.println(data.command, HEX); validCmd = false; //no valid command :(
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
      default: Serial.println(data.command, HEX); validCmd = false; //no valid command :(
    }
  }
  else
  {
    Serial.print(data.address, HEX);
    Serial.print(",");
    Serial.println(data.command, HEX);
    validCmd = false; //no valid command :(
  }

  //Did we get a valid command?
  if (validCmd == true)
  {
    LastCmdTime = millis(); //feed the dog
    lastcommand = data.command;
    lastaddress = data.address;
    executeCommand(command);
#ifdef DEBUGSERIAL
    Serial.println(command);
#endif
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
    if (getSerialCommand(readString, 15 ) == true) //not a timeout
    {
      //#ifdef DEBUGSERIAL Serial.println(readString);
      LastCmdTime = millis(); //reset the timeout timer
      //Serial.println(readString);
      executeCommand(readString); //this can also be used to feed in commands manually! executeCommand("FWD"); for example!
    }
  }

  //is IR remote data waiting?
  if (IRLremote.available())
  {
    // Get the new data from the remote
    decodeIRCommand();
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
