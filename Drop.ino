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
#define BEGIN_HOUR 23
#define BEGIN_MINUTE 46
int lastRunDate = 0;

//Electromechanical defines
#define ARM_ON HIGH
#define ARM_OFF LOW
#define ARM_EM 53

//Valve defines
#define VALVE_ON HIGH
#define VALVE_OFF LOW
#define VALVE_MAIN 23
#define VALVE_1 25
#define VALVE_2 27
#define VALVE_3 29
#define VALVE_4 31

//Time Defines
//Use Seconds
//Pump suggests 3.667 L/m
//5L ~= 82s
#define T_AREA_1 82  //Living wall
#define T_AREA_2 82  //Rail
#define T_AREA_3 82  //Herbs
#define T_AREA_4 82  //Between Door

//Motor
#define MOTOR_ON HIGH
#define MOTOR_OFF LOW
#define MOTOR 49

//debug defs
#define DEBUG
//#define VALVE_DEBUG
#define DATE_DEBUG

//Flow

//#define FLOW
#ifdef FLOW
float waterFlow = 0.0;
long flowTimeout = (600000);
#define FLOWPIN 2
#endif
#define F_AREA_1 10  //Living wall
#define F_AREA_2 10  //Rail
#define F_AREA_3 3  //Herbs
#define F_AREA_4 3  //Between Door

//--------------------------FUNCTIONS--------

#ifdef FLOW
void pulse() {
  waterFlow += 1.0 / 5880.0;
}
#endif FLOW

void emSafe() {
  digitalWrite(MOTOR, MOTOR_OFF);
  digitalWrite(VALVE_1, VALVE_OFF);
  digitalWrite(VALVE_2, VALVE_OFF);
  digitalWrite(VALVE_3, VALVE_OFF);
  digitalWrite(VALVE_4, VALVE_OFF);
  digitalWrite(VALVE_MAIN, VALVE_OFF);
  digitalWrite(ARM_EM, ARM_OFF);
}

void emBegin() { //This is meant to be called first
  pinMode(MOTOR, OUTPUT);
  pinMode(ARM_EM, OUTPUT);
  pinMode(VALVE_MAIN, OUTPUT);
  pinMode(VALVE_1, OUTPUT);
  pinMode(VALVE_2, OUTPUT);
  pinMode(VALVE_3, OUTPUT);
  pinMode(VALVE_4, OUTPUT);
  emSafe();

}

void emReady() {
  digitalWrite(MOTOR, MOTOR_OFF);
  digitalWrite(ARM_EM, ARM_ON);
  digitalWrite(VALVE_MAIN, VALVE_ON);
}



void emPOST() {
  digitalWrite(MOTOR, MOTOR_ON);
  digitalWrite(VALVE_1, VALVE_ON);
  digitalWrite(VALVE_2, VALVE_ON);
  digitalWrite(VALVE_3, VALVE_ON);
  digitalWrite(VALVE_4, VALVE_ON);
  digitalWrite(VALVE_MAIN, VALVE_ON);
  digitalWrite(ARM_EM, ARM_ON);
}

//----------------The Meat--------------------------------------

float emPumpArea(int area, int seconds, int flow) {
  unsigned long starttime = millis();
  unsigned long currenttime = millis();
  long interval = (1000 * long(seconds));

  #ifdef FLOW
  waterFlow=0;
  attachInterrupt(0, pulse, RISING); //Interrupt 0 is pin 1, execute pulse, rising edge
  interval=flowTimeout;
  #endif

  #ifdef DEBUG
    Serial.print("interval: ");
    Serial.println(interval);
  #endif

  while (currenttime - starttime <= interval) {
    digitalWrite(area, VALVE_ON);
    digitalWrite(MOTOR, MOTOR_ON);
    currenttime = millis();
    
    #ifdef FLOW
      displayCurrentTimePlusFlow();
      if(waterFlow >= float(flow)){
        break;
      }
    #endif

    
    #ifndef FLOW
    displayCurrentTimePlusSprinkler();
    #endif

#ifdef DEBUG
#ifdef VALVE_DEBUG
    Serial.print("CT: ");
    Serial.println(currenttime);
    Serial.print("Dif: ");
    Serial.println(currenttime - starttime);
    Serial.println(interval);
#endif
#endif
  }
  digitalWrite(area, VALVE_OFF);
  digitalWrite(MOTOR, MOTOR_OFF);

  #ifdef FLOW
  return waterFlow;
  #endif
}



void displayCurrentTime() {
  display.clearDisplay();

  int hour = clock.getHour(h12Flag, pmFlag);
  int minute = clock.getMinute();
  int second = clock.getSecond();

  display.setCursor(0, 0);
  if (hour <= 9) {
    display.print("0");
  }
  display.print(hour, DEC);
  display.print(":");
  if (minute <= 9) {
    display.print("0");
  }
  display.print(minute, DEC);
  display.print(":");
  if (second <= 9) {
    display.print("0");
  }
  display.print(second, DEC);
  display.display();
}

void displayCurrentTimePlusSprinkler() {
  display.clearDisplay();

  int hour = clock.getHour(h12Flag, pmFlag);
  int minute = clock.getMinute();
  int second = clock.getSecond();

  display.setCursor(0, 0);
  if (hour <= 9) {
    display.print("0");
  }
  display.print(hour, DEC);
  display.print(":");
  if (minute <= 9) {
    display.print("0");
  }
  display.print(minute, DEC);
  display.print(":");
  if (second <= 9) {
    display.print("0");
  }
  display.print(second, DEC);

  display.setCursor(0, 40);
  display.print("Sprinkle!");
  display.display();
}

#ifdef FLOW
void displayCurrentTimePlusFlow(){
  display.clearDisplay();

  int hour = clock.getHour(h12Flag, pmFlag);
  int minute = clock.getMinute();
  int second = clock.getSecond();

  display.setCursor(0, 0);
  if (hour <= 9) {
    display.print("0");
  }
  display.print(hour, DEC);
  display.print(":");
  if (minute <= 9) {
    display.print("0");
  }
  display.print(minute, DEC);
  display.print(":");
  if (second <= 9) {
    display.print("0");
  }
  display.print(second, DEC);

  display.setCursor(0, 40);
  display.print("Used ");
  display.print(waterFlow);
  display.print("L");
  display.display();
  
}
#endif


void setup() {
  emBegin();

  // put your setup code here, to run once:
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }


  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("RTC Clock!");
  display.display();
  delay(1000);


}

void loop() {
  // put your main code here, to run repeatedly:

  if (millis() % 100 == 1) {
    displayCurrentTime();

    if (clock.getHour(h12Flag, pmFlag) == BEGIN_HOUR) {
      if (clock.getMinute() == BEGIN_MINUTE) {
        if (lastRunDate != clock.getDate()) {
          //Sprinkler code would go here

          #ifdef DEBUG
            Serial.print("Prev: ");
            Serial.println(lastRunDate);
          #endif
          
          lastRunDate = clock.getDate();

          #ifdef DEBUG
            Serial.print("Cur: ");
            Serial.println(lastRunDate);
          #endif

          displayCurrentTimePlusSprinkler();
          emReady();
          emPumpArea(VALVE_1, T_AREA_1, F_AREA_1);
          emPumpArea(VALVE_2, T_AREA_2, F_AREA_2);
          emPumpArea(VALVE_3, T_AREA_3, F_AREA_3);
          emPumpArea(VALVE_4, T_AREA_4, F_AREA_4);
          emSafe();



        }
      }
    }



  }




}
