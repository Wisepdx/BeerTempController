//Sketch uses 28,988 bytes (101%) of program storage space. Maximum is 28,672 bytes.
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
#define background ST7735_BLACK // set default from AdaFruit GFX
#define charwidth 6 // set default from AdaFruit GFX
#define charheight 8  // set default from AdaFruit GFX

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
//String batchName = "unknown batch"; //Beer Name
int batchSize = 0;  // Beer Batch Size
int targetTemp = 60;  // Target Temp of Beer (In Fahrenheit)
int pumpStatus = 0; // Pump Status (0 = Off, 1 = On)
int peltStatus = 0; // Peltier Status (0 = Off, 2 = Heat, 1 = Cool)
String pumpStatusReadable = "Off";  // Make Pump Status Readable
String peltStatusReadable = "Off";  // Make Peltier Status Readable
float currentTemp;  // Current Temperature of Beer (In Fahrenheit)
float ambientTemp;  // Ambient Temperature of Room (In Fahrenheit)
float peltTemp; // Current Temperature of Peltier (In Fahrenheit)
unsigned long time; // current uptime in milliseconds
int tempDiff = 2; // Range at which temperature can drift from target 
int targetTempHigh = targetTemp + tempDiff; // High end of Temp Range
int targetTempLow = targetTemp - tempDiff;  // Low end of Temp Range

/*
------------------------------
SETUP LOOP START HERE   
------------------------------
*/

void setup() {
  //Initialize Bridge and Mailbox
  Bridge.begin();
  Mailbox.begin();
  //For Calculating uptime of Arduino/Batch
  Serial.begin(9600);
  FileSystem.begin();     
  //Initialize a ST7735S chip
  tft.initR(INITR_BLACKTAB);
  //Flips screen to Landscape
  tft.setRotation(3);
  //Start Temperature Sensors
  TempSensor.begin();
  //Motor Shield Temp Setup (Set to Off)
  motorOff(0);
  motorOff(1);
  //Display Screen
  displayScreen();
}

/*
-----------------------------
VOID LOOP START HERE   
-----------------------------
*/

void loop(){
  
  //MAILBOX
  String message;
  //if there is a message in the Mailbox
  if (Mailbox.messageAvailable()){
    // read all the messages present in the queue
    while (Mailbox.messageAvailable()){
      Mailbox.readMessage(message);
      Serial.println(message);
      for(int a=0;a<4;a++){
        digitalWrite(13, HIGH); //Reading Message, Turn LED 13 On
        delay(50);
        digitalWrite(13, LOW);
        delay(50);
      }
      String variableName = "";
      String variableValue = "";
      bool readingName = false;
      
      //loop though the message (in HTTP GET format)
      for(int i = 0; i < message.length();i++){
        //do somthing with each chr
        char currentCharacter = message[i];
        if (i == 0){
          //the first letter will always be the name
          readingName = true;
          variableName += currentCharacter;
        } else if(currentCharacter == '&'){
          //starting the next variable
          readingName = true;

          //if there is a name recorded
          if (variableName != ""){
            //write variables
            recordVariablesFromWeb(variableName, variableValue);
            //reset variables for possible next in string 'message'
            variableName = "";
            variableValue = "";
          } 
        } else if (currentCharacter == '='){
          //now reading the value
          readingName = false;
        } else {
          if(readingName == true){
            //add the current letter to the name
            variableName += currentCharacter; 
          } else{
            //add the current letter to the value
            variableValue += currentCharacter;
          }
        }
      }
      recordVariablesFromWeb(variableName, variableValue);
      digitalWrite(13, LOW); //After Message Read turn LED13 off
    }
    updateScreen();
  }
  delay(1000);
  //Read variables update and log to file
  readTemp(); // grab all temperatures from sensors and write to variables

  //Check temperatures against optimum settings and turn pump/peltier on or off 
  if (currentTemp < targetTempLow){

    //RUN PELTIER AS HEATER
      motorGo(1,CCW,255); //run peltier
      motorGo(0,CW,110); //run pump/fan
  } else if (currentTemp > targetTempHigh){

    //RUN PELTIER AS COOLER  
      motorGo(1,CW,255); //run peltier
      motorGo(0,CW,110); //run pump/fan
  }
    else if ((currentTemp <= targetTempHigh) && (currentTemp >= targetTempLow)){ 
      //TURN PELTIER AND FAN OFF
      motorOff(0); //turn off peltier
      motorOff(1); //turn off pump/fan
  }
    else{}
  
  delay(1000);
  //Update Screen
  updateScreen();

  // new data logger
  // make a json file [[x,y,z],[x,y,z]]
  // order of vars: timestamp, id, batch size, targetTemp, peltstatus, pumpStatus, currentTemp, ambientTemp

  writeDataFiles();

  //Update screen every minute for 5 minutes
  for (int i = 0; i < 5; i++){
    //delay for 1 minute
    delay(1000);
    //Update Screen
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
  // date is a command line utility to get the date and the time
  // in different formats depending on the additional parameter
  time.begin("date");
  time.addParameter("+%D %T");  // parameters: D for the complete date mm/dd/yy
  //  T for the time hh:mm:ss
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
  ambientTemp = TempSensor.getTempFByIndex(0);  //Black Small Sensor
  peltTemp = TempSensor.getTempFByIndex(1);   //Silver Small Sensor
  currentTemp = TempSensor.getTempFByIndex(2);  //(Metal Probe Sensor)
}

void writeDataFiles(){
  String logPath = "/mnt/sd/data/" + String(batchId) + ".csv"; // Local log variable for filename
  char charBuffer[logPath.length()+ 1];
  logPath.toCharArray(charBuffer, logPath.length()+ 1);
  
  String dataStream = getTimeStamp() + "," + batchId + "," + batchSize + "," + targetTemp + "," + currentTemp + "," + ambientTemp + "," + peltStatus + "," + pumpStatus;

  File dataFile = FileSystem.open(charBuffer, FILE_APPEND); // create file or append to it

  if (dataFile) {  
    dataFile.println(dataStream); //write enclosing JSON bracket
    dataFile.close(); //close the file
  }
/*
  logPath = "/mnt/sd/data/current.csv";
  char charBuffer1[logPath.length()+ 1];
  logPath.toCharArray(charBuffer1, logPath.length()+ 1);

  if (FileSystem.exists(charBuffer1)){
    FileSystem.remove(charBuffer1);
  }    
  File dataFile1 = FileSystem.open(charBuffer1, FILE_APPEND); // create file or append to it

  if (dataFile1) {  
    dataFile1.println(dataStream); //write enclosing JSON bracket
    dataFile1.close(); //close the file
  }
*/

}

/* original
void writeDataFile(){
  String logPath = "/mnt/sd/data/" + String(batchId) + ".json"; // Local log variable for filename
  char charBuffer[logPath.length()+ 1];
  logPath.toCharArray(charBuffer, logPath.length()+ 1);
  if (FileSystem.exists(charBuffer)){
    File dataFile = FileSystem.open(charBuffer, FILE_WRITE); // if the file already exists open file for writing
    unsigned long pos = dataFile.position(); //locate position
    pos = (pos - 1); // stepback a byte(character)
    dataFile.seek(pos); // move to the new location for writing
    // write to file [batchName,batchId,BatchSize,targetTemp,currentTemp,ambientTemp,peltStatus,pumpStatus]
    dataFile.println("["+ batchName + "," + String(batchId) + "," + String(batchSize) + "," + String(targetTemp) + "," + String(currentTemp) + "," + String(ambientTemp) + "," + String(peltStatus) + "," + String(pumpStatus) + "]"); //write enclosing JSON bracket
    dataFile.close(); //close the file
  } else{
    // if the file does not exist
    File dataFile = FileSystem.open(charBuffer, FILE_APPEND); // create file
    dataFile.println("["); //write opening JSON bracket
    // write to file [batchName,batchId,BatchSize,targetTemp,currentTemp,ambientTemp,peltStatus,pumpStatus]
    dataFile.println("["+ batchName + "," + String(batchId) + "," + String(batchSize) + "," + String(targetTemp) + "," + String(currentTemp) + "," + String(ambientTemp) + "," + String(peltStatus) + "," + String(pumpStatus) + "]"); //write enclosing JSON bracket
    dataFile.println("]"); //write closing JSON bracket
    dataFile.close(); //close the file
  }
}
*/
/*----- PARSE MAILBOX MESSAGE ----------------
--------------------------------------------*/
void recordVariablesFromWeb(String variableName, String variableValue){
  //Set values to their particular variables in this section
  //Parse Variables to the Proper Variable

  if(variableName == "batchid"){
    batchId = variableValue.toInt();
  }
  // else if(variableName == "batchname"){
  //   batchName = variableValue;
  // }
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
  tft.println("ID: ");
  tft.println("Batch Name: ");
  tft.println("Batch Size: ");
  tft.println("");
  tft.println("Current Temp: ");
  tft.println("Ambient Temp: ");
  tft.println("Target Temp: ");
  tft.println("Time: ");
  tft.println("");
  tft.println("Peltier Temp: ");
  tft.println("Pump Status: ");
  tft.println("Peltier Status: ");
}

void updateScreen(){
  //use this function to overwrite variables only and not the whole screen
  readTemp(); // read all temperatures from sensors
  tft.setCursor(0,0);
  tft.setTextSize(1);

  tft.setTextColor(ST7735_WHITE);
  tft.fillRect((charwidth*4),0,(160-(charwidth*4)),charheight,background);
  tft.setCursor((charwidth*4),0);
  tft.println(batchId);

  // tft.setTextColor(ST7735_WHITE);
  // tft.fillRect((charwidth*12),charheight,(160-(charwidth*12)),charheight,background);
  // tft.setCursor((charwidth*12),charheight);
  // tft.println(batchName);
  
  tft.fillRect((charwidth*12),charheight*2,(160-(charwidth*12)),charheight,background);
  tft.setCursor((charwidth*12),charheight*2);
  tft.println(String(batchSize) + " Gal");

  tft.fillRect((charwidth*14),(charheight*4),(160-(charwidth*14)),charheight,background);
  tft.setCursor((charwidth*14),charheight*4);
  tft.println(String(currentTemp) + " F");

  tft.fillRect((charwidth*14),(charheight*5),(160-(charwidth*14)),charheight,background);
  tft.setCursor((charwidth*14),charheight*5);
  tft.println(String(ambientTemp) + " F");

  tft.fillRect((charwidth*13),(charheight*6),(160-(charwidth*13)),charheight,background);
  tft.setCursor((charwidth*13),charheight*6);
  tft.println(String(targetTemp) + " F");

  tft.fillRect((charwidth*6),(charheight*7),(160-(charwidth*6)),charheight,background);
  tft.setCursor((charwidth*6),charheight*7);
  tft.println(String(targetTempHigh) + " " + String(targetTempLow));

  tft.fillRect((charwidth*14),(charheight*9),(160-(charwidth*14)),charheight,background);
  tft.setCursor((charwidth*14),charheight*9);
  tft.println(String(peltTemp) + " F");

  tft.fillRect((charwidth*13),(charheight*10),(160-(charwidth*13)),charheight,background);
  tft.setCursor((charwidth*13),charheight*10);
  tft.println(pumpStatusReadable);

  tft.fillRect((charwidth*16),(charheight*11),(160-(charwidth*16)),charheight,background);
  tft.setCursor((charwidth*16),charheight*11);
  tft.println(peltStatusReadable); 

}
/*
void printUptime(){
  
  //Uptime functions for Displaying Time on screen
  time = millis();
  long days=0;
  long hours=0;
  long mins=0;
  long secs=0;
  secs = time/1000; //convect milliseconds to seconds
  mins=secs/60; //convert seconds to minutes
  hours=mins/60; //convert minutes to hours
  days=hours/24; //convert hours to days
  secs=secs-(mins*60); //subtract the coverted seconds to minutes in order to display 59 secs max 
  mins=mins-(hours*60); //subtract the coverted minutes to hours in order to display 59 minutes max
  hours=hours-(days*24); //subtract the coverted hours to days in order to display 23 hours max
  //Display results
  if (days>0){ 
    // days will displayed only if value is greater than zero
    tft.print(days);
    tft.print(" d ");
  }
  tft.print(hours);
  tft.print(" h ");
  tft.print(mins);
  tft.print(" m ");
  
}
*/

/*----- MOTOR SHIELD FUNCTIONS ---------------
--------------------------------------------*/
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
