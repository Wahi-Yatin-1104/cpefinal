#include <LiquidCrystal.h>
#include <DHT.h>
#include <Stepper.h>
#include <RTClib.h>

// LCD Configuration
const int RS = 13, EN = 12, D4 = 11, D5 = 10, D6 = 9, D7 = 8;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// DHT Configuration
#define DHTPIN A0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Water level sensor pin
const int WATER_SENSOR_PIN = A1;

// LED pins
const int GREEN_LED_PIN = 7;
const int RED_LED_PIN = 6;
const int BLUE_LED_PIN = 4;
const int YELLOW_LED_PIN = 5; 

// Fan pins
#define FANIN A6
#define FANIN2 A7

const float TEMP_THRESHOLD = 19;

// Stepper motor pins
const int STEPS_PER_REV = 200;
const int IN1_PIN = A14; 
const int IN2_PIN = A15;
const int IN3_PIN = A13;
const int IN4_PIN = A12;

// Button pins
const int BUTTON_LEFT_PIN = 2;
const int BUTTON_RIGHT_PIN = 3;
const int CUSTOM_BUTTON_PIN = 18;
const int START_BUTTON_PIN = 19; 

Stepper stepper(STEPS_PER_REV, IN1_PIN, IN3_PIN, IN2_PIN, IN4_PIN);

bool fanRunning = false;
volatile bool systemDisabled = false; 

unsigned long lcdUpdateMillis = 0;
const unsigned long lcdUpdateInterval = 1000;

RTC_DS1307 rtc;

void setup() {
 Serial.begin(9600);
 lcd.begin(16, 2);
 dht.begin();
 pinMode(WATER_SENSOR_PIN, INPUT);

 pinMode(GREEN_LED_PIN, OUTPUT);
 pinMode(RED_LED_PIN, OUTPUT);
 pinMode(BLUE_LED_PIN, OUTPUT);
 pinMode(YELLOW_LED_PIN, OUTPUT); 
 pinMode(FANIN, OUTPUT);
 pinMode(FANIN2, OUTPUT);

 pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);
 pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
 pinMode(CUSTOM_BUTTON_PIN, INPUT_PULLUP);
 pinMode(START_BUTTON_PIN, INPUT_PULLUP); 
 attachInterrupt(digitalPinToInterrupt(START_BUTTON_PIN), toggleSystemState, CHANGE); 

 digitalWrite(GREEN_LED_PIN, LOW);
 digitalWrite(RED_LED_PIN, LOW);
 digitalWrite(BLUE_LED_PIN, LOW);
 digitalWrite(YELLOW_LED_PIN, LOW);

 stepper.setSpeed(60);
  //RTC
  #ifndef ESP8266
  while (!Serial); 
#endif

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
 if (systemDisabled) {
  // When the system is disabled
  digitalWrite(YELLOW_LED_PIN, HIGH); 
  Serial.println("DISABLED - ");
  displayTimeState();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Disabled");
  return; 
 } else {
  digitalWrite(YELLOW_LED_PIN, LOW); 
 }

 digitalWrite(YELLOW_LED_PIN, LOW);

 int waterSensorValue = analogRead(WATER_SENSOR_PIN);

 float temperature = dht.readTemperature();

 if (waterSensorValue <= 100) {
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
  Serial.println("ERROR - ");
  displayTimeState();
  digitalWrite(BLUE_LED_PIN, LOW);
  digitalWrite(FANIN, LOW);
  digitalWrite(FANIN2, LOW);
  displayError("Error: EMPTY!");
} else if (temperature > TEMP_THRESHOLD) {
  if (digitalRead(RED_LED_PIN) == HIGH) {
    if (digitalRead(CUSTOM_BUTTON_PIN) == LOW && waterSensorValue > 100) {
      digitalWrite(RED_LED_PIN, LOW);
    }
  } else {
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, HIGH);
    Serial.println("RUNNING - ");
    displayTimeState();
    digitalWrite(FANIN, HIGH);
    digitalWrite(FANIN2, LOW);
    fanRunning = true;
    displayTempAndHumidity();
  }
} else {
  if (digitalRead(RED_LED_PIN) == HIGH) {
    if (digitalRead(CUSTOM_BUTTON_PIN) == LOW && waterSensorValue > 100) {
      digitalWrite(RED_LED_PIN, LOW);
    }
  } else {
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println("IDLE - ");
    displayTimeState();
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(FANIN, LOW);
    digitalWrite(FANIN2, LOW);
    fanRunning = false;
    displayTempAndHumidity();
  }
}


 int leftButtonState = digitalRead(BUTTON_LEFT_PIN);
 int rightButtonState = digitalRead(BUTTON_RIGHT_PIN);

 if (leftButtonState == LOW) {
  rotateStepper(-1);
 } else if (rightButtonState == LOW) {
  rotateStepper(1);
 }

 unsigned long currentMillis = millis();
 if (currentMillis - lcdUpdateMillis >= lcdUpdateInterval) {
  lcdUpdateMillis = currentMillis;
 }
}

void toggleSystemState() {
 if (digitalRead(START_BUTTON_PIN) == LOW) {
  systemDisabled = !systemDisabled; // Toggle system state

  if (systemDisabled) {
   // Turn off all LEDs except the yellow one
   digitalWrite(GREEN_LED_PIN, LOW);
   digitalWrite(RED_LED_PIN, LOW);
   digitalWrite(BLUE_LED_PIN, LOW);
    
   // Turn off the fan
   digitalWrite(FANIN, LOW);
   digitalWrite(FANIN2, LOW);
   fanRunning = false;
  }
 }
}

void displayTimeState(){
  DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

}


void displayTempAndHumidity() {
 if (systemDisabled) return; 

 float temperature = dht.readTemperature();
 float humidity = dht.readHumidity();
 lcd.setCursor(0, 0);
 lcd.print("Temp: ");
 lcd.print(temperature);
 lcd.print(" C");
 lcd.setCursor(0, 1);
 lcd.print("Humidity: ");
 lcd.print(humidity);
 lcd.print(" %");
}


void displayError(const char* message) {
 lcd.clear();
 lcd.print(message);
}

void rotateStepper(int direction) {
 if (systemDisabled) return;

 if (direction == -1) {
  stepper.step(STEPS_PER_REV / 10);
 } else if (direction == 1) {
  stepper.step(-STEPS_PER_REV / 10);
 }
}


void disableSystemISR() {
 systemDisabled = !systemDisabled;
 if (systemDisabled) {
  displayDisabled();
 }
}

void displayDisabled() {
 lcd.clear();
 lcd.print("Disabled");
}