// currently displaying start time instead of uptime
// removed pelt temperature sensor due to space requirements
// removing some display variables for space reasons. will eventually remove

/*-------------- Libraries -----------------
--------------------------------------------*/
#include <Adafruit_GFX.h>  // Core graphics library
#include <Adafruit_ST7735.h>  // Hardware-specific library
#include <SPI.h>  // Serial Peripheral Interface library
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
int batchId = 1;  // beer ID (always make 3 digit)
String batchName = "unknown batch"; // beer name
int batchSize = 0;  // beer batch size
int targetTemp = 62;  // target temp of beer (In Fahrenheit)
int pumpStatus = 0; // pump Status (0 = Off, 1 = On)
int peltStatus = 0; // peltier status (0 = Off, 1 = Cool, 2 = Heat)
float currentTemp;  // current temperature of beer (In Fahrenheit)
float ambientTemp;  // ambient temperature of room (In Fahrenheit)
unsigned long time; // current uptime in milliseconds
String upTime = ""; // holder for uptime
int tempDiff = 1; // range at which temperature can drift from target
int targetTempHigh = targetTemp + tempDiff; // high end of temp range
int targetTempLow = targetTemp - tempDiff;  // low end of temp range

/*
------------------------------
SETUP LOOP START HERE
------------------------------
*/

void setup() {
  // initialize Bridge and Mailbox
  Bridge.begin();
  Mailbox.begin();
  // for Calculating uptime of Arduino/Batch
  Serial.begin(9600);
  FileSystem.begin();
  // initialize a ST7735S chip
  tft.initR(INITR_BLACKTAB);
  // flips screen to Landscape
  tft.setRotation(3);
  // start Temperature Sensors
  TempSensor.begin();
  // motor Shield Temp Setup (Set to Off)
  motorOff(0);
  motorOff(1);
  // display Screen
  displayScreen();
}

/*
-----------------------------
VOID LOOP START HERE
-----------------------------
*/

void loop(){

  // check/read the mailbox
  // mailboxCheck();

  // update the screen
  updateScreen();

  // check temperatures against optimum settings and turn pump/peltier on or off, update screen with new statuses and temps
  // includes function to write datafiles and update screen every minute
  motorCheck();
  
  // update the screen
  updateScreen();
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
  // date is a command line utility to get the date and the time
  // in different formats depending on the additional parameter
  time.begin("date");
  time.addParameter("+%D %T");  // parameters: D for the complete date mm/dd/yy , T for the time hh:mm:ss
  time.run();  // run the command
  // read the output of the command
  while (time.available() > 0) {
    char c = time.read();
    if (c != '\n')
      result += c;
  }
  return result;
}

void readTemp(){
  TempSensor.requestTemperatures();
  ambientTemp = TempSensor.getTempFByIndex(0);  // Black Small Sensor
  currentTemp = TempSensor.getTempFByIndex(2);  // (Metal Probe Sensor)
}

/*----- WRITE DATAFILES --------------------
--------------------------------------------*/
void writeDataFiles(){
  digitalWrite(13, HIGH);

  // batch file (#.csv)
  String logPath = "/mnt/sd/arduino/www/data/" + String(batchId) + ".csv"; // Local log variable for filename
  char charBuffer[logPath.length()+ 1];
  logPath.toCharArray(charBuffer, logPath.length()+ 1);

  String header = "Date,ID,Name,Size,Target,Current,Ambient,Pump,Pelt";
  String dataStream = getTimeStamp() + "," + batchId + "," + batchName + "," + batchSize + "," + targetTemp + "," + currentTemp + "," + ambientTemp + "," + pumpStatus + "," + peltStatus;

  if (FileSystem.exists(charBuffer)){
    File dataFile = FileSystem.open(charBuffer, FILE_APPEND); // create file or append to it
    if (dataFile) {
      dataFile.println(dataStream); // write csv file
      dataFile.close(); // close the file
    }
  } else{
    File dataFile = FileSystem.open(charBuffer, FILE_APPEND); // create file or append to it
    if (dataFile) {
      dataFile.println(header); // write csv file
      dataFile.println(dataStream); // write csv file
      dataFile.close(); // close the file
    }
  }

  // current file (current.csv)
  logPath = "/mnt/sd/arduino/www/data/current.csv";
  char charBuffer1[logPath.length()+ 1];
  logPath.toCharArray(charBuffer1, logPath.length()+ 1);

  if (FileSystem.exists(charBuffer1)){
    FileSystem.remove(charBuffer1);
  }
  File dataFile1 = FileSystem.open(charBuffer1, FILE_APPEND); // create file or append to it

  if (dataFile1) {
    dataFile1.println(header); //write csv file
    dataFile1.println(dataStream); //write csv file
    dataFile1.close(); //close the file
  }
  digitalWrite(13, LOW);
  for(int a=0;a<4;a++){
        digitalWrite(13, HIGH); // reading Message, Turn LED 13 On
        delay(50);
        digitalWrite(13, LOW);
        delay(50);
  }
}

/*----- PARSE MAILBOX MESSAGE ----------------
--------------------------------------------*/
void mailboxCheck(){
  String message;
  // if there is a message in the Mailbox
  if (Mailbox.messageAvailable()){
    
    for (int i=1;i<4;i++){
      digitalWrite(13,HIGH);
      delay(1000);
      digitalWrite(13,LOW);
    }
    
    digitalWrite(13,HIGH);
    // read all the messages present in the queue
    while (Mailbox.messageAvailable()){
      Mailbox.readMessage(message);
        // serial.println(message);
      String variableName = "";
      String variableValue = "";
      bool readingName = false;

      // loop though the message (in HTTP GET format)
      for(int i = 0; i < message.length();i++){
        // do somthing with each chr
        char currentCharacter = message[i];
        if (i == 0){
          // the first letter will always be the name
          readingName = true;
          variableName += currentCharacter;
        } else if(currentCharacter == '&'){
          // starting the next variable
          readingName = true;

          // if there is a name recorded
          if (variableName != ""){
            // Write variables
            recordVariablesFromWeb(variableName, variableValue);
            // Reset variables for possible next in string 'message'
            variableName = "";
            variableValue = "";
          }
        } else if (currentCharacter == '='){
          // now reading the value
          readingName = false;
        } else {
          if(readingName == true){
            // add the current letter to the name
            variableName += currentCharacter;
          } else{
            // add the current letter to the value
            variableValue += currentCharacter;
          }
        }
      }
      recordVariablesFromWeb(variableName, variableValue);
      digitalWrite(13, LOW); // after Message Read turn LED13 off
    }
  }
}

void recordVariablesFromWeb(String variableName, String variableValue){
  // set values to their particular variables in this section
  // parse Variables to the Proper Variable

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
    // set high/low range of target temperature range
  }
  targetTempHigh = targetTemp + tempDiff;
  targetTempLow = targetTemp - tempDiff;
}

/*----- DISPLAY FUNCTIONS --------------------
--------------------------------------------*/
void displayScreen(){
  // use this function to erase screen then replace with line headers
  // follow by update screen function to fill in variables

  // reset cursor to top left of screen
  tft.setCursor(0,0);
  // fill screen with black
  tft.fillScreen(ST7735_BLACK);

  // set row headers
  tft.setTextSize(1);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.println("     ID:");
  tft.println("Current:");
  tft.println("Ambient:");
  tft.println(" Target:");
  tft.println("Peltier:");
}

void overwriteScreenText(int Column, int Row, String DisplayString){
  // overwrite screen text
  tft.setCursor(Column*6,Row*8);
  tft.setTextSize(1);
  tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
  tft.setCursor(48,Row*8);
  tft.print(DisplayString);
}

void updateScreen(){
  //use this function to overwrite variables only and not the whole screen

  // grab all temperatures from sensors and write to variables
  readTemp();
  // reset cursor to top left of screen
  tft.setCursor(0,0);

  // fill in variables for rows
  overwriteScreenText(0,0, String(batchId));
  overwriteScreenText(0,1, String(currentTemp) + " F"); // 0,4
  overwriteScreenText(0,2, String(ambientTemp) + " F"); // 0,5
  overwriteScreenText(0,3, String(targetTemp) + " F"); // 0,6
  overwriteScreenText(0,4, String(peltStatus)); // 0 off ; 1 cool ; 2 heat  || 0,10

}

void updateScreenWriteFilesWait(){

  // Check Mailbox
  mailboxCheck();
  
  // make a csv file for current batch (overwrites) and one for numbered batch (appended)
  // order of vars: timestamp, id, name, batch size, targetTemp, currentTemp, ambientTemp
  writeDataFiles();
  
  // update the screen
  updateScreen();

  // delay for 1 minute
  delay(10000);
  
  
}


/*----- MOTOR SHIELD FUNCTIONS ---------------
--------------------------------------------*/
void motorCheck(){
  // grab all temperatures from sensors and write to variables
  readTemp();
  // check temperature against target and alter motors accordingly
  if (currentTemp < targetTempLow){
    // run peltier as heater
    motorGo(1,CW,220); // peltier
    motorGo(0,CW,150); // pump/fan
    // after starting motors, run the following until currentTemp is higher than target Temp
    while(currentTemp < targetTemp){
      updateScreenWriteFilesWait();
    }
  } 
  else 
if (currentTemp > targetTempHigh){
    // run peltier as cooler
    motorGo(1,CCW,220); // peltier
    motorGo(0,CW,150); // pump/fan
    // after starting motors, run the following until currentTemp is lower than target Temp
    while(currentTemp > targetTemp){
      updateScreenWriteFilesWait();
    }
  }
  else{
    motorOff(0); // peltier
    motorOff(1); // pump/fan
    //  after stopping motors, run the following until currentTemp is outside the target temp range (high/low)
    while(currentTemp < targetTempHigh && currentTemp > targetTempLow){
      updateScreenWriteFilesWait();
    }
  }
}

void motorOff(int motor){
  // initialize braked
  for (int i=0; i<2; i++){
    digitalWrite(inApin[i], LOW);
    digitalWrite(inBpin[i], LOW);
  }
  if (motor == 0){
    pumpStatus = 0;
  }else if (motor == 1){
    peltStatus = 0;
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
    if (direct <= 4){
      // Set inA[motor]
      if (direct <=1){
        digitalWrite(inApin[motor], HIGH);
      }
      else{
        digitalWrite(inApin[motor], LOW);
      }
      // set inB[motor]
      if ((direct == 0)||(direct == 2)){
        digitalWrite(inBpin[motor], HIGH);
      }
      else{
        digitalWrite(inBpin[motor], LOW);
      }
      analogWrite(pwmpin[motor], pwm);
    }
    // set status
    if (motor == 0){
      // set pump status
      pumpStatus = 1;
    }else if (motor == 1){
      // set pelt status based on motor direction
      if (direct == CW){
        peltStatus = 2;
      }else if (direct == CCW){
        peltStatus = 1;
      }
    }
  }
}
