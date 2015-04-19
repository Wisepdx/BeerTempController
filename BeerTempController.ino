// Need to fix uptime script or instead just display the time the sketch started
// Removed pelt temperature sensor
/*-------------- Libraries -----------------
--------------------------------------------*/
#include <Adafruit_GFX.h>  // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library
#include <SPI.h>  // Serial Peripheral Interface library (motor controller?? //**)
#include <DallasTemperature.h>  // Temp Sensor library
#include <OneWire.h>  // For One Wire Temp Sensor Library
#include <Mailbox.h>  // Mailbox Library
#include <FileIO.h>  // File IO Library for writing to files

/*--------------- Defines ------------------
--------------------------------------------*/

// TFT Display
#define TFT_CS 10
#define TFT_RST 0  // you can also connect this to the Arduino reset in which case, set this #define pin to 0!
#define TFT_DC 8

// TFT Display Pins
#define TFT_SCLK 13 // set these to be whatever pins you like!
#define TFT_MISO 12 // set these to be whatever pins you like!
#define TFT_MOSI 11 // set these to be whatever pins you like!

// TFT Default Background and Text Size
// #define background ST7735_BLACK // set default from AdaFruit GFX
// #define charwidth 6 // set default from AdaFruit GFX
// #define charheight 8  // set default from AdaFruit GFX

// Temperature Sensors
#define ALLTEMP 0
#define CURRENT 3
#define PELTIER 2
#define AMBIENT 1

// Motor Controller
#define BRAKEVCC 0
#define CW 1
#define CCW 2
#define BRAKEGND 3
#define CS_THRESHOLD 100

// One Wire Temperature Sensors using Digital Pin 2
OneWire TempSensorPin(2);
DallasTemperature TempSensor(&TempSensorPin);

// MUST USE THIS OPTION WITH THE YUN FOR DISPLAY TO WORK PROPERLY
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

/*
Motor Controller
xxx[0] controls '1' outputs (PUMP)
xxx[1] controls '2' outputs (PELTIER)
*/
int inApin[2] = {7, 4};  // INA: Clockwise input
int inBpin[2] = {3, 9};  // INB: Counter-clockwise input {8, 9};
int pwmpin[2] = {5, 6};  // PWM input
int cspin[2] = {A2, A3}; // CS: Current sense ANALOG input
int enpin[2] = {A0, A1}; // EN: Status of switches output (Analog pin)

/*-------------- Datapoints ----------------
--------------------------------------------*/
int batchId = 1;  // Beer ID (always make 3 digit)
String batchName = "unknown batch"; //Beer Name
int batchSize = 0;  // Beer Batch Size
int targetTemp = 60;  // Target Temp of Beer (In Fahrenheit)
int pumpStatus = 0; // Pump Status (0 = Off, 1 = On)
int peltStatus = 0; // Peltier Status (0 = Off, 2 = Heat, 1 = Cool)
String pumpStatusReadable = "Off";  // Make Pump Status Readable
String peltStatusReadable = "Off";  // Make Peltier Status Readable
float currentTemp;  // Current Temperature of Beer (In Fahrenheit)
float ambientTemp;  // Ambient Temperature of Room (In Fahrenheit)
//float peltTemp; // Current Temperature of Peltier (In Fahrenheit)
unsigned long time; // current uptime in milliseconds
String upTime = ""; // holder for uptime
int tempDiff = 1; // Range at which temperature can drift from target 
int targetTempHigh = targetTemp + tempDiff; // High end of Temp Range
int targetTempLow = targetTemp - tempDiff;  // Low end of Temp Range

/*
------------------------------
SETUP LOOP START HERE   
------------------------------
*/

void setup() {
  // Initialize Bridge and Mailbox
  Bridge.begin();
  Mailbox.begin();
  // For Calculating uptime of Arduino/Batch
  Serial.begin(9600);
  FileSystem.begin();     
  // Initialize a ST7735S chip
  tft.initR(INITR_BLACKTAB);
  // Flips screen to Landscape
  tft.setRotation(3);
  // Start Temperature Sensors
  TempSensor.begin();
  // Motor Shield Temp Setup (Set to Off)
  motorOff(0);
  motorOff(1);
  // Display Screen
  displayScreen();
}

/*
-----------------------------
VOID LOOP START HERE   
-----------------------------
*/

void loop(){
  
  // Check/Read the mailbox
  mailboxCheck();

  // Check temperatures against optimum settings and turn pump/peltier on or off, update screen with new statuses and temps 
  motorCheck();
  
  // Log variables to files
  // Make a csv file for current batch (overwrites) and one for numbered batch (appended)
  // Order of vars: timestamp, id, name, batch size, targetTemp, currentTemp, ambientTemp
  writeDataFiles();

  //Update screen every minute for 5 minutes, then loop to top
  for (int i = 0; i < 5; i++){
    // Delay for 1 minute
    delay(6000);
    // Update Screen
    updateScreen();
  }  
}

/*
------------------------------------
FUNCTIONS START BELOW HERE   
------------------------------------
*/

/*----- RANDOM FUNCTIONS WITHOUT A HOME-------
--------------------------------------------*/
String getTimeStamp() {
  String result;
  Process time;
  // Date is a command line utility to get the date and the time
  // In different formats depending on the additional parameter
  time.begin("date");
  time.addParameter("+%D %T");  // parameters: D for the complete date mm/dd/yy
  //  T for the time hh:mm:ss
  time.run();  // run the command
  // Read the output of the command
  while (time.available() > 0) {
    char c = time.read();
    if (c != '\n')
      result += c;
  }
  return result;
}

void readTemp(){
  TempSensor.requestTemperatures(); 
  ambientTemp = TempSensor.getTempFByIndex(0);  //Black Small Sensor
  // peltTemp = TempSensor.getTempFByIndex(1);   //Silver Small Sensor
  currentTemp = TempSensor.getTempFByIndex(2);  //(Metal Probe Sensor)
}

/*----- WRITE DATAFILES --------------------
--------------------------------------------*/
void writeDataFiles(){
  digitalWrite(13, HIGH);
  String logPath = "/mnt/sd/data/" + String(batchId) + ".csv"; // Local log variable for filename
  char charBuffer[logPath.length()+ 1];
  logPath.toCharArray(charBuffer, logPath.length()+ 1);
  
  String dataStream = getTimeStamp() + "," + batchId + "," + batchName + "," + batchSize + "," + targetTemp + "," + currentTemp + "," + ambientTemp;

  File dataFile = FileSystem.open(charBuffer, FILE_APPEND); // create file or append to it

  if (dataFile) {  
    dataFile.println(dataStream); //write csv file
    dataFile.close(); //close the file
  }
  // Current File
  logPath = "/mnt/sd/data/current.csv";
  char charBuffer1[logPath.length()+ 1];
  logPath.toCharArray(charBuffer1, logPath.length()+ 1);

  if (FileSystem.exists(charBuffer1)){
    FileSystem.remove(charBuffer1);
  }    
  File dataFile1 = FileSystem.open(charBuffer1, FILE_APPEND); // create file or append to it

  if (dataFile1) {  
    dataFile1.println(dataStream); //write csv file
    dataFile1.close(); //close the file
  }
  digitalWrite(13, LOW);
  for(int a=0;a<4;a++){
        digitalWrite(13, HIGH); //Reading Message, Turn LED 13 On
        delay(50);
        digitalWrite(13, LOW);
        delay(50);
  }

}

/*----- PARSE MAILBOX MESSAGE ----------------
--------------------------------------------*/
void mailboxCheck(){
  String message;
  // If there is a message in the Mailbox
  if (Mailbox.messageAvailable()){
    digitalWrite(13,HIGH);
    // Read all the messages present in the queue
    while (Mailbox.messageAvailable()){
      Mailbox.readMessage(message);
      // Serial.println(message);
      String variableName = "";
      String variableValue = "";
      bool readingName = false;
      
      // Loop though the message (in HTTP GET format)
      for(int i = 0; i < message.length();i++){
        // Do somthing with each chr
        char currentCharacter = message[i];
        if (i == 0){
          // The first letter will always be the name
          readingName = true;
          variableName += currentCharacter;
        } else if(currentCharacter == '&'){
          // Starting the next variable
          readingName = true;

          // If there is a name recorded
          if (variableName != ""){
            // Write variables
            recordVariablesFromWeb(variableName, variableValue);
            // Reset variables for possible next in string 'message'
            variableName = "";
            variableValue = "";
          } 
        } else if (currentCharacter == '='){
          // Now reading the value
          readingName = false;
        } else {
          if(readingName == true){
            // Add the current letter to the name
            variableName += currentCharacter; 
          } else{
            // Add the current letter to the value
            variableValue += currentCharacter;
          }
        }
      }
      recordVariablesFromWeb(variableName, variableValue);
      digitalWrite(13, LOW); // After Message Read turn LED13 off
    }
  }  
  updateScreen();
}

void recordVariablesFromWeb(String variableName, String variableValue){
  // Set values to their particular variables in this section
  // Parse Variables to the Proper Variable

  if(variableName == "batchid"){
    batchId = variableValue.toInt();
  }
  else if(variableName == "batchname"){
    batchName = variableValue;
  }
  else if(variableName == "batchsize"){
    batchSize = variableValue.toInt();
  }
  else if(variableName == "tempdiff"){
    tempDiff = variableValue.toInt();
  }
  else if(variableName == "targettemp"){
    targetTemp = variableValue.toInt();
    //set high/low range of target temperature range
    targetTempHigh = targetTemp + tempDiff;
    targetTempLow = targetTemp - tempDiff;
  }
}

/*----- DISPLAY FUNCTIONS --------------------
--------------------------------------------*/        
void displayScreen(){
  //use this function to erase screen then replace with line headers
  //follow by update screen functino to fill in variables  
  tft.setCursor(0,0);
  //Fill Screen with Black
  tft.fillScreen(ST7735_BLACK);
  //Set Line Headers
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN);
  tft.println("     ID: ");
  tft.println("   Name: ");
  tft.println("   Size: ");
  tft.println();
  //tft.setCursor(32,0);
  tft.println("Current: ");
  tft.println("Ambient: ");
  tft.println(" Target: ");
  tft.print("Started: ");
  tft.setTextColor(ST7735_WHITE);
  tft.println(String(getTimeStamp()));
  //tft.setCursor(72,0);
  //tft.println();
  tft.setTextColor(ST7735_GREEN);
  tft.println("   Pump: ");
  tft.println("Peltier: ");
}

void overwriteScreenText(int Column, int Row, String DisplayString){
  //int sWidth = DisplayString.length();
  tft.setCursor(Column*6,Row*8);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE);
  tft.fillRect(54,Row*8,(160-(6*DisplayString.length())),8,ST7735_BLACK);
  tft.setCursor(54,Row*8);
  tft.print(DisplayString);
}

void updateScreen(){
  //use this function to overwrite variables only and not the whole screen
  readTemp(); // read all temperatures from sensors
  tft.setCursor(0,0);
  
  overwriteScreenText(0,0, String(batchId));
  overwriteScreenText(0,1, batchName);
  overwriteScreenText(0,2, String(batchSize) + " Gal");

  overwriteScreenText(0,4, String(currentTemp) + " F");
  overwriteScreenText(0,5, String(ambientTemp) + " F");
  overwriteScreenText(0,6, String(targetTemp) + " F");
  // Display Started time instead Time. Set in displayScreen()
  overwriteScreenText(0,9, pumpStatusReadable);
  overwriteScreenText(0,10, peltStatusReadable);

}

/*----- MOTOR SHIELD FUNCTIONS ---------------
--------------------------------------------*/
void motorCheck(){
  // Grab all temperatures from sensors and write to variables
  readTemp(); 
  // Check temperatures and alter motors accordingly
  if (currentTemp < targetTempLow){
    // Run peltier as heater
    motorGo(1,CCW,255); // peltier
    motorGo(0,CW,110); // pump/fan
  } else if (currentTemp > targetTempHigh){
    // Run peltier as cooler  
    motorGo(1,CW,255); // peltier
    motorGo(0,CW,110); // pump/fan
  }
  else{
    motorOff(0); // peltier
    motorOff(1); // pump/fan
  }
  // Update Screen
  updateScreen();
}

void motorOff(int motor){
  // Initialize braked
  for (int i=0; i<2; i++){
    digitalWrite(inApin[i], LOW);
    digitalWrite(inBpin[i], LOW);
  }
  if (motor == 0){
    pumpStatus = 0;
    pumpStatusReadable = "Off";
  }else if (motor == 1){
    peltStatus = 0;
    peltStatusReadable = "Off";
  }
  analogWrite(pwmpin[motor], 0);
}

void motorGo(uint8_t motor, uint8_t direct, uint8_t pwm){
  /* motorGo() will set a motor going in a specific direction the motor will continue going in that direction, at that speed until told to do otherwise.
  - motor: this should be either 0 or 1, will selet which of the two motors to be controlled
  - direct: Should be between 0 and 3, with the following result
  0: Brake to VCC
  1: Clockwise ...or use CW
  2: CounterClockwise ...or use CCW
  3: Brake to GND 
  - pwm: should be a value between ? and 1023, higher the number, the faster it'll go
  */
  if (motor <= 1){
    if (direct <=4){
      // Set inA[motor]
      if (direct <=1){
        digitalWrite(inApin[motor], HIGH);
      }
      else{
        digitalWrite(inApin[motor], LOW);
      }
      // Set inB[motor]
      if ((direct==0)||(direct==2)){
        digitalWrite(inBpin[motor], HIGH);
      }
      else{
        digitalWrite(inBpin[motor], LOW);
      }
      analogWrite(pwmpin[motor], pwm);
    }
    // set pump status
    if (motor == 0){
      pumpStatus = 1;
      pumpStatusReadable = "On";
    }else if (motor == 1){
      // set pelt status based on motor direction
      if (direct == CW){
        peltStatus = 1;
        peltStatusReadable = "Cool";
      }else if (direct == CCW){
        peltStatus = 2;
        peltStatusReadable = "Heat";
      }
    }
  }
}
