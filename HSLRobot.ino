#include <string.h>

// pins for motors -- These need to be analog capable pins
#define L_FWD_PIN 3
#define L_REV_PIN 5
#define R_FWD_PIN 6
#define R_REV_PIN 9

void setup()
{
  // initialize serial:
  Serial.begin(9600);
  // make the pins outputs:
  pinMode(L_FWD_PIN, OUTPUT);
  pinMode(L_REV_PIN, OUTPUT);
  pinMode(R_FWD_PIN, OUTPUT);
  pinMode(R_REV_PIN, OUTPUT);
  Serial.println("Greetings from HeatSync Labs!");
}

bool getSerialCommand(String &readString, int timeoutVal = 200) //Note that readString is passed by reference!
{
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
  int bVal = 0;
  //Right now the protocol looks like FWD=50; REV=30; with SINGLE numbers. 
  //If we need multiple numbers (like 2-axis raw values), reccomand, RAW=50,30; format NOT IMPLEMENTED YET
  //Something like this
  //val1 = readString.indexOf(',');  //finds location of first ,
  //angle = readString.substring(0, val1);   //captures first data String
  
  if (command.startsWith("FWD=") )
  {
    //Turn motors forward at a given speed.
    command.remove(0,4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
    Serial.print("FWD=");
    Serial.println(bVal, DEC);

    analogWrite(L_REV_PIN, 0);
    analogWrite(R_REV_PIN, 0);
    analogWrite(L_FWD_PIN, bVal);
    analogWrite(R_FWD_PIN, bVal);
  }
  else if (command.startsWith("REV=") )
  {
   //Turn motors forward at a given speed.
    command.remove(0,4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
    Serial.print("REV=");
    Serial.println(bVal, DEC);

    analogWrite(L_FWD_PIN, 0);
    analogWrite(R_FWD_PIN, 0); 
    analogWrite(L_REV_PIN, bVal);
    analogWrite(R_REV_PIN, bVal);   
  }
  else if (command.startsWith("LFT=") )
  {
    //Turn motors forward at a given speed.
    command.remove(0,4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
    Serial.print("LFT=");
    Serial.println(bVal, DEC);

    //Write zeros first to make sure we don't accidentally short an H-bridge leg
    analogWrite(L_FWD_PIN, 0);
    analogWrite(R_REV_PIN, 0);
    analogWrite(L_REV_PIN, bVal);
    analogWrite(R_FWD_PIN, bVal);
    
  }
  else if (command.startsWith("RGT=") )
  {
    //Turn motors forward at a given speed.
    command.remove(0,4); //remove the command, the leftovers should be the value
    bVal = command.toInt();
    Serial.print("RGT=");
    Serial.println(bVal, DEC);

    //Write zeros first to make sure we don't accidentally short an H-bridge leg
    analogWrite(L_REV_PIN, 0);
    analogWrite(R_FWD_PIN, 0);
    analogWrite(L_FWD_PIN, bVal);
    analogWrite(R_REV_PIN, bVal);
  }
  else if (command.startsWith("RAW=") )
  {    
    int bVal2 = 0,idx=0;
    String subString;
    //Control F/B and L/R at the same time from an analog driver. Range is from -255 to +255 with two values
    //Syntax is Raw=127,-100;
    //command.remove(0,4); //remove the command, the leftovers should be the value
    idx = command.indexOf(',');  //finds location of first
    if(idx <= 0 || idx == (command.length()-1))
      return; //Bail, no comma or no second number
    
    subString = command.substring(4, idx);
    bVal = subString.toInt();   //captures first data String
    subString = command.substring(idx+1,command.length());
    bVal2 = subString.toInt();   //captures first data String
    
    Serial.print("RAW=");
    Serial.print(bVal, DEC);
    Serial.print(',');
    Serial.print(bVal2, DEC);
    Serial.print(':');
    Serial.print(command.length(),HEX);
    Serial.print(',');
    Serial.println(idx,HEX);
    
    if(bVal >= 0 && bVal2 >= 0) //we're moving forward and right
      {
      Serial.println("FWR");  
      if(bVal-bVal2 >= 0)
      {        
        analogWrite(R_REV_PIN, 0);
        analogWrite(R_FWD_PIN, constrain(bVal-bVal2,0,255));
      }
      else
      {        
        analogWrite(R_FWD_PIN, 0);
        analogWrite(R_REV_PIN, constrain(bVal2-bVal,0,255));        
      }
      analogWrite(L_REV_PIN, 0);      
      analogWrite(L_FWD_PIN, constrain(bVal+bVal2,0,255));      
      }
    else if(bVal <= 0 && bVal2 >= 0) //we're moving backward and right
    {
      bVal *= -1;
      Serial.println("RVR");  
      if(bVal-bVal2 >= 0)
      {
        analogWrite(R_FWD_PIN, 0);
        analogWrite(R_REV_PIN, constrain(bVal-bVal2,0,255));
      }
      else
      {
        analogWrite(R_REV_PIN, 0);
        analogWrite(R_FWD_PIN, constrain(bVal2-bVal,0,255));        
      }
      analogWrite(L_FWD_PIN, 0);      
      analogWrite(L_REV_PIN, constrain(bVal+bVal2,0,255));      
    }
    else if(bVal >= 0 && bVal2 <= 0) //we're moving forward and left
    {
      bVal2 *= -1;
      Serial.println("FWL");  
      if(bVal-bVal2 >= 0)
      {
        analogWrite(L_REV_PIN, 0);
        analogWrite(L_FWD_PIN, constrain(bVal-bVal2,0,255));
      }
      else
      {
        analogWrite(L_FWD_PIN, 0);
        analogWrite(L_REV_PIN, constrain(bVal2-bVal,0,255));        
      }
      analogWrite(R_REV_PIN, 0);      
      analogWrite(R_FWD_PIN, constrain(bVal+bVal2,0,255));      
    }
    else if(bVal <= 0 && bVal2 <= 0) //we're moving backward and left
    {
      bVal *= -1;
      bVal2 *= -1;
      Serial.println("RVL");  
      if(bVal-bVal2 >= 0)
      {
        analogWrite(L_FWD_PIN, 0);
        analogWrite(L_REV_PIN, constrain(bVal-bVal2,0,255));
      }
      else
      {
        analogWrite(L_REV_PIN, 0);
        analogWrite(L_FWD_PIN, constrain(bVal2-bVal,0,255));        
      }
      analogWrite(R_FWD_PIN, 0);      
      analogWrite(R_REV_PIN, constrain(bVal+bVal2,0,255));      
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
}

void loop()
{
  String readString = ""; //the incoming command will be put here.
  if (getSerialCommand(readString) == true) //not a timeout
    {
    //Serial.println(readString);  
    executeCommand(readString); //this can also be used to feed in commands manually! executeCommand("FWD"); for example!
    }
  else
    executeCommand("STP");
}








