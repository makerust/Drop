#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DS3231.h>
#include <Wire.h>

DS3231 clock;

bool century = false;
bool h12Flag;
bool pmFlag;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Time Set
#define BEGIN_HOUR 11
#define BEGIN_MINUTE 30
int lastRunDate = 0;

//Electromechanical defines
#define ARM_ON 1
#define ARM_OFF 0
#define ARM_EM 53

//Valve defines
#define VALVE_ON 1
#define VALVE_OFF 0
#define VALVE_MAIN 23
#define VALVE_1 25
#define VALVE_2 27
#define VALVE_3 29
#define VALVE_4 31

//Time Defines
//Use Seconds
//Pump suggests 3.667 L/m
//5L ~= 82s
#define T_AREA_1 82
#define T_AREA_2 82
#define T_AREA_3 82
#define T_AREA_4 82

//Motor
#define MOTOR_ON 1
#define MOTOR_OFF 0
#define MOTOR 49

//debug defs
#define DEBUG
#define VALVE_DEBUG
#define DATE_DEBUG

void emBegin(){ //This is meant to be called first
  digitalWrite(MOTOR, MOTOR_OFF);
  digitalWrite(ARM_EM, ARM_ON);
  digitalWrite(VALVE_MAIN, VALVE_ON);
}

void emEnd(){
  digitalWrite(MOTOR, MOTOR_OFF);
  digitalWrite(VALVE_1, VALVE_OFF);
  digitalWrite(VALVE_2, VALVE_OFF);
  digitalWrite(VALVE_3, VALVE_OFF);
  digitalWrite(VALVE_4, VALVE_OFF);
  digitalWrite(VALVE_MAIN, VALVE_OFF);
  digitalWrite(ARM_EM, ARM_OFF);
}

void emPumpArea(int area, int seconds){
  unsigned long starttime = millis();
  unsigned long currenttime = millis();
  const long interval = (seconds*1000);
  
  while(currenttime-starttime <= interval){
    digitalWrite(area, VALVE_ON);
    digitalWrite(MOTOR, MOTOR_ON);
    currenttime=millis();
    //If you decide to allow this to update time it goes here
    
    #ifdef DEBUG
    #ifdef VALVE_DEBUG
    Serial.print("CT: ");
    Serial.println(currenttime);
    Serial.print("Dif: ");
    Serial.println(currenttime-starttime);
    #endif
    #endif
  }
  
}



void displayCurrentTime(){
     display.clearDisplay();
     
    int hour = clock.getHour(h12Flag, pmFlag);
    int minute = clock.getMinute();
    int second = clock.getSecond();
    int lastRunDate = (clock.getDate()-1);
    
    display.setCursor(0,0);
    if(hour<=9){
      display.print("0");
    }
    display.print(hour, DEC);
    display.print(":");
    if(minute<=9){
      display.print("0");
    }
    display.print(minute, DEC);
    display.print(":");
    if(second<=9){
      display.print("0");
    }
    display.print(second, DEC);
    display.display();
}

void displayCurrentTimePlusSprinkler(){
    display.clearDisplay();
     
    int hour = clock.getHour(h12Flag, pmFlag);
    int minute = clock.getMinute();
    int second = clock.getSecond();
    lastRunDate = (clock.getDate()-1);
    
    display.setCursor(0,0);
    if(hour<=9){
      display.print("0");
    }
    display.print(hour, DEC);
    display.print(":");
    if(minute<=9){
      display.print("0");
    }
    display.print(minute, DEC);
    display.print(":");
    if(second<=9){
      display.print("0");
    }
    display.print(second, DEC);

    display.setCursor(0,40);
    display.print("Sprinkle!");
    display.display();    
}




void setup() {
   emEnd();
  // put your setup code here, to run once:
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }


  display.clearDisplay(); 
  display.setTextSize(2); 
  display.setTextColor(WHITE);
  display.setCursor(0,10);
  display.println("RTC Clock!");
  display.display();
  delay(1000);
  
  
}

void loop() {
  // put your main code here, to run repeatedly:

 if(millis()%100==1){ 
  displayCurrentTime();
  
  if(clock.getHour(h12Flag, pmFlag) == BEGIN_HOUR){
    if(clock.getMinute() == BEGIN_MINUTE){
      if(lastRunDate!=clock.getDate()){
        //Sprinkler code would go here
        lastRunDate=clock.getDate();
        for(int i =0; i<2400; i++){
          displayCurrentTimePlusSprinkler();
          emBegin();
          emPumpArea(1, T_AREA_1);
          emPumpArea(2, T_AREA_2);
          emPumpArea(3, T_AREA_3);
          emPumpArea(3, T_AREA_3);
          emEnd();
          delay(250); //delay plus increment should last 10 minutes        
        }
      }
    }
  }
  


 }
 

  

}
