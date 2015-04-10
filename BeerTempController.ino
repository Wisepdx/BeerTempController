/*
Arduino Project

To Work on

- Writing to file on SD Card - see below work after removing wait for console
- Transferring variables from webpage to arduino - see below woks after removing wait for console

- run a 5 minute logging script while also running temperature code
- heat cool water within a range of temperatures
- check if i need lines appended with //**      

To ADD:
*IN PROCESS* Merge in files from webcode2 which is makes use of the TempWebPanel and mailbox examples
Merge in the data logger function to write 


//******************************CODE BELOW**************************************
*/

#include <Adafruit_GFX.h>				//Core graphics library
#include <Adafruit_ST7735.h>		//Hardware-specific library
#include <SPI.h> 								//Serial Peripheral Interface library (motor controller?? //**)
#include <DallasTemperature.h> 	//Temp Sensor library
#include <OneWire.h> 						//For One Wire Temp Sensor Library
#include <Mailbox.h> 						//Mailbox Library
//#include <Bridge.h> 					//**Yun Bridge Library included in Mailbox Library
#include <YunServer.h> 					//Yun Server Library
#include <YunClient.h> 					//YunClient Library

// SET ALL DEFINES HERE
	
	// For the breakout, you can use any 2 or 3 pins
	// These pins will also work for the 1.8" TFT shield
	#define TFT_CS     10
	#define TFT_RST    0  // you can also connect this to the Arduino reset in which case, set this #define pin to 0!
	#define TFT_DC     8

	// Define Display Pins
	#define TFT_SCLK 13   //set these to be whatever pins you like!
	#define TFT_MISO 12   //set these to be whatever pins you like!
	#define TFT_MOSI 11   //set these to be whatever pins you like!

	// Define Background and Text Size for text refreshing
	#define Background ST7735_BLACK
	#define charwidth 6 	// from AdaFruit GFX
	#define charheight 8 	// from AdaFruit GFX

	// temperature Defines
	#define ALLTEMP 0
	#define CURRENT 3
	#define PELTIER 2
	#define AMBIENT 1

	// motor Defines
	#define BRAKEVCC 0
	#define CW   1
	#define CCW  2
	#define BRAKEGND 3
	#define CS_THRESHOLD 100

	// One Wire Temperature Sensors using Digital Pin 2
	OneWire TempSensorPin(2);
	DallasTemperature TempSensor(&TempSensorPin);

	// MUST USE THIS OPTION WITH THE YUN FOR DISPLAY TO WORK PROPERLY
	Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

YunServer server;

/*
VNH2SP30 pin definitions
xxx[0] controls '1' outputs (PUMP)
xxx[1] controls '2' outputs (PELTIER)
*/
int inApin[2] = {7, 4};  //INA: Clockwise input
int inBpin[2] = {3, 9};  //INB: Counter-clockwise input {8, 9};
int pwmpin[2] = {5, 6};  //PWM input
int cspin[2] = {A2, A3}; //CS: Current sense ANALOG input
int enpin[2] = {A0, A1}; //EN: Status of switches output (Analog pin)

// DataPoints and defaults - Some of these will be replaced by inputs from webpage 
int batchId = 0;               	 //Beer ID
int batchName = "unknown batch"; //Beer Name
int batchSize = 0;               //Beer Batch Size
int targetTemp = 0;              //Target Temp of Beer (In Fahrenheit)
int pumpStatus = 0;              //Pump Status (0 = Off, 1 = On)
int peltStatus = 0;              //Peltier Status (0 = Off, 1 = Heat, 2 = Cool)
String pumpStatusReadable = "";  //Make Pump Status Readable
String peltStatusReadable = "";  //Make Peltier Status Readable
float currentTemp;               //Current Temperature of Beer (In Fahrenheit)
float ambientTemp;               //Ambient Temperature of Room (In Fahrenheit)
float peltTemp;                  //Current Temperature of Peltier (In Fahrenheit)
unsigned long time;              //Set UpTime

String startString; //Time that the sketch started
  
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
	//Listen for incoming connection only from localhost
	//(no one from the external network could connect)
	server.listenOnLocalhost();
	server.begin();

	//get the time that this sketch started:
	Process startTime;
	startTime.runShellCommand("date");
	while (startTime.available()) {
		char c = startTime.read();
		startString += c;
	}
}

/*
-----------------------------
	VOID LOOP START HERE   
-----------------------------
*/

void loop{
	//MAILBOX
	String message;
	//if there is a message in the Mailbox
	if (Mailbox.messageAvailable()){
    	// read all the messages present in the queue
    	while (Mailbox.messageAvailable()){
    	Mailbox.readMessage(message);
    	digitalWrite(13, HIGH); //Reading Message, Turn LED 13 On
    
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
		          }
		          else{
		          	//add the current letter to the value
		            	variableValue += currentCharacter;
		          }
        	}
      	}
      
      		recordVariablesFromWeb(variableName, variableValue);   
      		//delay(1000);
      		digitalWrite(13, LOW); //After Message Read turn LED13 off
    	}
	}

	//Read variables update and log to file
	readTemp(ALLTEMP); // grab all temperatures from sensors and write to variables
		
	//Check temperatures against optimum settings and turn pump/peltier on or off
	//set high low range of targetTemp  (maybe move into void setup)
	var targetTempHigh = targetTemp + 1; //High Range
	var targetTempLow = targetTemp - 1;  //Low Range
	
	//TURN PELTIER AND FAN OFF	
	if ((CurrentTemp <= targetTempHigh) && (currentTemp >= targetTempLow)){	
		motorOff(0); //turn off peltier
		motorOff(1); //turn off pump/fan
		
		//update variables 
		peltStatus = 0;
		pumpStatus = 0;
	}
	//RUN PELTIER AS HEATER
	else if (currentTemp < targetTemp){
		if (peltStatus = 0){				
			motorGo(1,CW,255); //run peltier
			motorGo(0,CW,110); //run pump/fan
			
			//** update Status variables should be happening in motorGo function 
			//peltStatus = 1;
			//pumpStatus = 1;
		}
		else{}
	}
	//RUN PELTIER AS COOLER
	else if (currentTemp >= targetTemp){
		if (peltStatus = 0){	
			motorGo(1,CW,255); //run peltier
			motorGo(0,CW,110); //run pump/fan
				
			//** update Status variables should be happening in motorGo function 
			//peltStatus = 2;
			//pumpStatus = 1;
		}
		else{}
	}

	//Update Screen
	updateScreen();

	//Update Webpage
	
	// Get clients coming from server
	YunClient client = server.accept();
  
	// There is a new client?
	if (client) {
		// read the command
		String command = client.readString();
		command.trim();  //kill whitespace
		Serial.println(command);
		// is "temperature" command?
		if (command == "temperature") {

		  // get the time from the server:
		  getCurentTime(); //** moved out to a function - does this still work with the variable call on the next line / further down?
		  Serial.println(timeString); //** do i need this if not reading the serial connection??
		  //Get Current Temp - Should already be happening in updateScreen() function above //**
		  // int currentTemp = TempSensor.getTempFByIndex(0); //** see comment on above line

		  // print the output to the webpage:
		  client.print("Current time on the Yún: ");
		  client.println(currentTime);
		  
		  client.print("<br>");
		  client.print("<br><b>Brew ID:</b> " + String(batchId));
		  client.print("<br><b>Batch Name:</b> " + batchName);
		  client.print("<br><b>Batch Size:</b> " + String(batchSize) + " Gallons");
		  client.print("<br>");
		  client.print("<br><b>Current Temp:</b> " + String(currentTemp) + " degrees F");
		  client.print("<br><b>Target Temp:</b> " + String(targetTemp) + " degrees F");
		  client.print("<br><b>Peltier Temp:</b> " + String(peltTemp) + " degrees F");
		  client.print("<br><b>Ambient Temp:</b> " + String(ambientTemp) + " degrees F");
		  client.print("<br>");
		  client.print("<br><b>Peltier Status:</b> " + peltStatusReadable);
		  client.print("<br><b>Pump Status:</b> " + pumpStatusReadble);  
		}

	    // Close connection and free resources.
	    client.stop();
 	}

	//Update Datafile
	// To build out later //**

	//Update screen every minute for 5 minutes
	for (i=0, i > 5, i++){
		//delay for 1 minute
		delay(60000);
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

void getCurrentTime(){
	//get the current time/date from the server
	Process time;
  time.runShellCommand("date");
  String currentTime = "";
  while (time.available()) {
    char c = time.read();
    currentTime += c;
  }
}

/*----- PARSE MAILBOX MESSAGE ----------------
  --------------------------------------------*/

void recordVariablesFromWeb(String variableName, String variableValue){
	//Set values to their particular variables in this section

	//Parse Variables to the Proper Variable
    
  if(variableName == "batchid"){
    batchId = variableValue.toInt();
  }
  else if(variableName == "batchname"){
    batchName = variableValue;
  }
  else if(variableName == "batchsize"){
    batchSize = variableValue.toInt();
  }
  else if(variableName == "targettemp"){
    targetTemp = variableValue.toInt();
  }
}

/*----- DISPLAY FUNCTIONS --------------------
  --------------------------------------------*/				
				
void displayScreen(){
	//use this function to erase screen then replace with lien headers
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

	readTemp(ALLTEMP); // read all temperatures from sensors
	tft.setCursor(0,0);
	tft.setTextSize(1);
	tft.setTextColor(ST7735_WHITE);
	tft.fillRect((charwidth*4),0,(160-(charwidth*4)),charheight,Background);
	tft.setCursor((charwidth*4),0);
	tft.println(batchId);
	//Added check that it works //**
	tft.fillRect((charwidth*12),charheight,(160-(charwidth*12)),charheight,Background);
	tft.setCursor((charwidth*12),charheight);
	tft.println(batchName);
	//END Added Section //**
	tft.fillRect((charwidth*12),charheight*2,(160-(charwidth*12)),charheight,Background);
	tft.setCursor((charwidth*12),charheight*2);
	tft.println(String(batchSize) + " Gallons");
	tft.fillRect((charwidth*14),(charheight*4),(160-(charwidth*14)),charheight,Background);
	tft.setCursor((charwidth*14),charheight*4);
	tft.println(String(currentTemp) + " F");
	tft.fillRect((charwidth*14),(charheight*5),(160-(charwidth*14)),charheight,Background);
	tft.setCursor((charwidth*14),charheight*5);
	tft.println(String(ambientTemp) + " F");
	tft.fillRect((charwidth*13),(charheight*6),(160-(charwidth*13)),charheight,Background);
	tft.setCursor((charwidth*13),charheight*6);
	tft.println(String(targetTemp) + " F");
	tft.fillRect((charwidth*6),(charheight*7),(160-(charwidth*6)),charheight,Background);
	tft.setCursor((charwidth*6),charheight*7);
	time = millis();
	printUptime();
	tft.fillRect((charwidth*14),(charheight*9),(160-(charwidth*14)),charheight,Background);
	tft.setCursor((charwidth*14),charheight*9);
	tft.println(String(peltTemp) + " F");
	tft.fillRect((charwidth*13),(charheight*10),(160-(charwidth*13)),charheight,Background);
	tft.setCursor((charwidth*13),charheight*10);
	tft.println(pumpStatusReadable);
	tft.fillRect((charwidth*16),(charheight*11),(160-(charwidth*16)),charheight,Background);
	tft.setCursor((charwidth*16),charheight*11);
	tft.println(pumpStatusReadable);  
}

void printUptime(){
	//Uptime functions for Displaying Time on screen
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
		tft.print(" days :");
	}
	tft.print(hours);
	if  (hours == 1){
		tft.print(" hour ");
	}
	else{  
		tft.print(" hours ");
	}
	if (days<1){
		tft.print(mins);
		if (mins == 1){
			tft.print(" minute");
		}
		else{
			tft.print(" minutes");
		} 
	}
}

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
    	pumpStatusReadable = "Off"
  	}else if (motor == 1){
    	peltStatus = 0;
    	peltStatusReadable = "Off"
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
      		if (direct <=1)
        		digitalWrite(inApin[motor], HIGH);
      		else
        		digitalWrite(inApin[motor], LOW);
      		// Set inB[motor]
      		if ((direct==0)||(direct==2))
        		digitalWrite(inBpin[motor], HIGH);
      		else
        	digitalWrite(inBpin[motor], LOW);
      		analogWrite(pwmpin[motor], pwm);
    	}
    		// set pump status
    	if (motor == 0){
      		pumpStatus = 1;
      		pumpStatusReadable = "On"
    	}else if (motor == 1){
      		// set pelt status based on motor direction
      		if (direct == CW){
        		peltStatus = 1;
        		peltStatusReadable = "Heat"
      		}else if (direct == CCW){
        		peltStatus = 2;
        		peltStatusReadable = "Cool"
      		}
    	}
  	}
}

		
