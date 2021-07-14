#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DS3231.h>
#include <Wire.h>

typedef struct 
{
  /* data */
  int beginHour;
  int beginMinute;
} hourMinute;


//watering Time Set
hourMinute waterAlarm = {6, 00};// watering time, format (h)h,mm
//end watering time set

DS3231 clock;

bool century = false;
bool h12Flag;
bool pmFlag;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3c ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int lastRunDate = 0;



//init a placeholder
hourMinute lastFinishTime = {0,0};


//Electromechanical defines
#define ARM_ON HIGH
#define ARM_OFF LOW
#define ARM_EM 53

//Valve defines
#define VALVE_ON HIGH
#define VALVE_OFF LOW

//Motor
#define MOTOR_ON HIGH
#define MOTOR_OFF LOW
#define MOTOR 49

//debug defs
#define DEBUG
//#define VALVE_DEBUG
#define DATE_DEBUG

//Flow
#define FLOW
#ifdef FLOW
float waterFlow = 0.0;
long flowTimeout = (600000);
#define FLOWPIN 2
#endif


typedef struct{
  int valvePin; //GPIO pin
  int valveTime; //backup watering time in seconds
  int valveTimeout; //Timeout for watering
  float valveVolume; //watering volume in liters
  float valveVolumeCorrection; //Correction for tube volume
} valveArea;

valveArea balconySystem[5];


//--------------------------FUNCTIONS--------

#ifdef FLOW
void pulse() {
  waterFlow += 1.0 / 2037.5;
  //0.35 Reported liters produced 1.0 Actual liters
  //prev value 5880.0
  //Trial 1 value 2058.1
  //Trail 2 value is 1852.3
  //Trial 3 value 2037.5
}
#endif

void emSafe() {
  digitalWrite(MOTOR, MOTOR_OFF);
  for(int i=0; i<4; i++){
    digitalWrite(balconySystem[i].valvePin, VALVE_OFF);
  }
  digitalWrite(ARM_EM, ARM_OFF);
}

void emBegin() { //This is meant to be called first
  pinMode(MOTOR, OUTPUT);
  pinMode(ARM_EM, OUTPUT);
    for(int i=0; i<4; i++){
    pinMode(balconySystem[i].valvePin, OUTPUT);
  }
  emSafe();

}

void emReady() {
  digitalWrite(MOTOR, MOTOR_OFF);
  digitalWrite(ARM_EM, ARM_ON);
}

void emPOST() {
  digitalWrite(MOTOR, MOTOR_ON);
  digitalWrite(ARM_EM, ARM_ON);
  for(int i=0; i<4; i++){
    digitalWrite(balconySystem[i].valvePin, VALVE_ON);
  }  
}

//----------------The Meat--------------------------------------

float emPumpArea(valveArea* sys) {
  unsigned long starttime = millis();
  unsigned long currenttime = millis();
  long seconds;
  long interval;

  #ifndef FLOW
  seconds = sys->valveTime;
  interval = (1000 * long(seconds));
  #endif

  #ifdef FLOW
  waterFlow=0;
  attachInterrupt(0, pulse, RISING); //Interrupt 0 is pin 1, execute pulse, rising edge
  seconds = sys->valveTimeout;
  interval = (1000 * long(seconds));
  #endif

  #ifdef DEBUG
    Serial.print("interval: ");
    Serial.println(interval);
  #endif

  while (currenttime - starttime <= interval) {
    digitalWrite(sys->valvePin, VALVE_ON);
    digitalWrite(MOTOR, MOTOR_ON);
    currenttime = millis();
    
    #ifdef FLOW
      displayCurrentTimePlusFlow();
      if(waterFlow >= float(sys->valveVolume+sys->valveVolumeCorrection)){
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
  
  digitalWrite(sys->valvePin, VALVE_OFF);
  digitalWrite(MOTOR, MOTOR_OFF);

  #ifdef FLOW
    return waterFlow;
    detachInterrupt(0); //Interrupt 0 is pin 1
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

void displayCurrentTimePlusAlarm(hourMinute* today) {
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
  display.print("W: ");
  display.print(today->beginHour);
  display.print(":");
  display.print(today->beginMinute);
  display.display();
}

void displayCurrentTimePlusLastRun(hourMinute* todayEnd) {
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
  display.print("end: ");
  display.print(todayEnd->beginHour);
  display.print(":");
  display.print(todayEnd->beginMinute);
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
  

  //Define the main balconysystem struct
  //Areas listed
  //This section defines the watering amount et al.
  //
  //Living Wall
  balconySystem[0].valvePin = 31;//31
  balconySystem[0].valveTime = 82;//default if flow sensing doesn't work
  balconySystem[0].valveTimeout = 1200;//20 minutes in S
  balconySystem[0].valveVolume = 10;//Watering amount in liters
  balconySystem[0].valveVolumeCorrection =1;// correction for the tube volume
  //Rail
  balconySystem[1].valvePin = 29;
  balconySystem[1].valveTime = 82;
  balconySystem[1].valveTimeout = 1200;
  balconySystem[1].valveVolume = 10;
  balconySystem[1].valveVolumeCorrection = 0.75;
  //Herbs
  balconySystem[2].valvePin = 27;
  balconySystem[2].valveTime = 82;
  balconySystem[2].valveTimeout = 1200;
  balconySystem[2].valveVolume = 3;
  balconySystem[2].valveVolumeCorrection = 0;
  //Between Walls
  balconySystem[3].valvePin = 25;
  balconySystem[3].valveTime = 82;
  balconySystem[3].valveTimeout = 1200;
  balconySystem[3].valveVolume = 3;
  balconySystem[3].valveVolumeCorrection = 0;
  //Unused
  balconySystem[4].valvePin = 23;
  balconySystem[4].valveTime = 0;
  balconySystem[4].valveTimeout =0;
  balconySystem[4].valveVolume = 0;
  balconySystem[4].valveVolumeCorrection = 0;

  emBegin();

  int shortStoreDate = 0;
  int shortStoreHour = 0;
  int shortStoreMinute = 0;
  

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
  display.println("Drop v1.0");
  display.display();
  delay(1000);


}

void loop() {
  // put your main code here, to run repeatedly:

  if (millis() % 100 == 1) {
    if (lastRunDate != clock.getDate()) {
      displayCurrentTimePlusAlarm(&waterAlarm);
    }
    else{
      displayCurrentTimePlusLastRun(&lastFinishTime);
    }

    if (clock.getHour(h12Flag, pmFlag) == waterAlarm.beginHour) {
      if (clock.getMinute() == waterAlarm.beginMinute) {
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
          if(1==clock.getDate()%2){//the following are watered every other day
            emPumpArea(&balconySystem[0]);
            emPumpArea(&balconySystem[3]);
          }
          emPumpArea(&balconySystem[1]);
          emPumpArea(&balconySystem[2]);
          emSafe();
          
          lastFinishTime.beginMinute=clock.getMinute();
          lastFinishTime.beginHour=clock.getHour(h12Flag, pmFlag);



        }
      }
    }



  }




}
