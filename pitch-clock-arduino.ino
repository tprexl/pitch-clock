#include <Adafruit_WS2801.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

const int analogInPin = A0;    // Analog input pin that the potentiometer is attached to
const int startPin = 8;        // Start closer to ground
const int stopPin = 9;         // Stop closer to ground
const int resetPin = 10;       // Reset closer to ground
const int dataPin = 11;        // WS2801 data pin (white)
const int clockPin = 12;       // WS2801 clock pin (green)

LiquidCrystal_I2C lcd(0x3F,16,2);  // initialize 1602 lcd connected via i2c
Adafruit_WS2801 strip = Adafruit_WS2801(92, dataPin, clockPin); // initialize WS2801 chain

int rawSensorValue = 0;        // Initialize variable for raw reading from poti
int setTime = 0;               // Initialize variable for mapped poti value
boolean isRunning = false;     // Initialize boolean for timer status
int countFromSeconds = 0;      // Initialize variable for "set"
int remainingSeconds = 0;      // Initialize varibale to store remaining seconds
unsigned long lastMillis = 0;  // Initialize variable for store milis value when starting the timer 
uint32_t LEDColor = 0;
boolean flashToggle = false;   // Crude hack for a flash toggle for the remaining 10 secs

byte digit[10][30] = {          // map to turn digits into led matrix
  {1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
  {0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},
  {0,0,0,0,1,1,1,1,1,1,1,1,0,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1},
  {1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1},
  {1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1}
};

byte all_digits[92];

void setup() {

  pinMode(startPin, INPUT_PULLUP);      // Initiate pins for buttons
  pinMode(stopPin, INPUT_PULLUP);
  pinMode(resetPin, INPUT_PULLUP);


  lcd.init();                           // Initiate I2C 1602 display
  lcd.backlight();                      // Switch on backlight

  lcd.setCursor(0,0);                   // Print all static characters and put something on the screen to start with
  lcd.print("Set: 0:00 Min.");
  lcd.setCursor(0,1);
  lcd.print("---- 0:00 ----");

  strip.begin();                        // Initialize and blank LED strip
  strip.show();


  readAndOutputSetTime();               // Initially read input value
  doReset();                            // Get timer ready to go

  Serial.begin(57600);                  // ### DEBUG ### - Start serial console
}


// Helper function to round to full numbers
int roundTo(unsigned int number, byte to)
{
  return ((number+to/2)/to)*to;
}

// Helper function to turn rgb values to the color object needed by WS2801 library
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

// Helper function to get the minute digit from an interger of seconds
int getMinutes(int seconds)
{
  return int(seconds / 60);
}

// Helper function to get the ten-seconds digit from a interger of seconds
int getTenSeconds(int seconds)
{
  return int(seconds % 60 / 10);
}

// Helper function to get the seconds digit from a interger of seconds
int getSeconds(int seconds)
{
  return int(seconds % 60 % 10);
}

void checkInputs() {
}

void readAndOutputSetTime() {
  // Get value from poti
  rawSensorValue = analogRead(analogInPin);
  // Map values from analog read to timer scope and round to 30 seconds
  setTime = roundTo(map(rawSensorValue, 0, 1023, 15, 575), 30);

  // Output
  lcd.setCursor(5,0);
  lcd.print(getMinutes(setTime));
  lcd.print(":");
  lcd.print(getTenSeconds(setTime));
  lcd.print(getSeconds(setTime));
}

// Check buttons and initiate adequate action depending on state
void checkButtons() {
  if ((digitalRead(startPin) == 0) && (isRunning == false) && (remainingSeconds > 0)) {
    doStart();
  }
  if ((digitalRead(stopPin) == 0) && (isRunning == true)) {
    doStop();
  }
  if (digitalRead(resetPin) == 0) {
    doStop();
    doReset();
  }

}

void doStart() {
  isRunning = true;      // Change state
  lcd.setCursor(0,1);    // Show asterisk
  lcd.print("****");
  lcd.setCursor(10,1);
  lcd.print("****");
  lastMillis = millis();  // Set millis comparison value
}


void doStop() {
  isRunning = false;      // Change state
  lcd.setCursor(0,1);     // Show dashes
  lcd.print("----");
  lcd.setCursor(10,1);
  lcd.print("----");
  countFromSeconds = remainingSeconds;  // Set new value for seconds to count from
}


void doReset() {
  countFromSeconds = setTime;           // Set value according to set
  remainingSeconds = countFromSeconds;  // Reset timer
  lcd.setCursor(5,1);                   // Show new time in display
  lcd.print(getMinutes(remainingSeconds));
  lcd.print(":");
  lcd.print(getTenSeconds(remainingSeconds));
  lcd.print(getSeconds(remainingSeconds));
  refreshLEDS(remainingSeconds);
}

void countDown () {
  if (isRunning == true) {
    int secondsGone = int((millis() - lastMillis) / 1000);
    if (secondsGone > (countFromSeconds - remainingSeconds)) {
      remainingSeconds = countFromSeconds - secondsGone;
      lcd.setCursor(5,1);
      lcd.print(getMinutes(remainingSeconds));
      lcd.print(":");
      lcd.print(getTenSeconds(remainingSeconds));
      lcd.print(getSeconds(remainingSeconds));

      if (remainingSeconds == 0) {
        isRunning = false;
      }

      refreshLEDS(remainingSeconds);
      flashToggle = false;
    }
    if ((remainingSeconds < 10) && (flashToggle == false) && ((millis() - lastMillis) - ((countFromSeconds - remainingSeconds) * 1000UL) > 500)) {
      switchOffLEDS();
      flashToggle = true;
    }
  }
}

void refreshLEDS(int seconds) {
  if (seconds >= 60) {
    LEDColor = Color(0, 255, 0);
  }
  else if (seconds < 60 && seconds >= 10) {
    LEDColor = Color(255, 255, 0);
  }
  else {
     LEDColor = Color(255, 0, 0);
  }

  int array_counter = 0;
  int array_secs = getSeconds(seconds);
  int array_tensecs = getTenSeconds(seconds);
  int array_mins = getMinutes(seconds);

  for (int i = 0; i<sizeof(digit[array_mins]); i++) {
    if (array_mins > 0) {
      all_digits[array_counter] = digit[array_mins][i];
    }
    else
    {
      all_digits[array_counter] = 0;
    }
    array_counter = array_counter + 1;
  }

  all_digits[array_counter] = 1;
  array_counter = array_counter + 1;
  all_digits[array_counter] = 1;
  array_counter = array_counter + 1;

  for (int i = 0; i<sizeof(digit[array_tensecs]); i++) {
    all_digits[array_counter] = digit[array_tensecs][i];
    array_counter = array_counter + 1;
  }

  for (int i = 0; i<sizeof(digit[array_secs]); i++) {
  all_digits[array_counter] = digit[array_secs][i];
  array_counter = array_counter + 1;
  }

  for (int i = 0; i<sizeof(all_digits); i++) {
    if (all_digits[i] == 1) {
      strip.setPixelColor(i, LEDColor);  
    }
    else {
      strip.setPixelColor(i, 0);
    }
  }

  strip.show();
}


void switchOffLEDS() {
  for (int i = 0; i<sizeof(all_digits); i++) {
    strip.setPixelColor(i, 0);  
  }
  strip.show();
}

void loop() {

  readAndOutputSetTime();
  checkButtons();
  countDown();
  delay(100);

  
}