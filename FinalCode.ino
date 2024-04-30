#include <Adafruit_NAU7802.h>
#include "Adafruit_VL53L0X.h"
#include <LiquidCrystal_I2C.h>

Adafruit_NAU7802 nau;
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
Adafruit_VL53L0X lox = Adafruit_VL53L0X();


#define MOTOR_IN1 37
#define MOTOR_IN2 36
#define lockPin 39

char readingOutSd[200];
int tsHours, tsMinutes, tsSeconds, msTime;

int vol;
int32_t pres;

void setup() {
  Serial.begin(115200);

  //setup the locking mechanism
  pinMode(lockPin, OUTPUT);

  //setup the motor 
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);

  //setup pressure sensor 
  Serial.println("NAU7802");
  if (! nau.begin()) {
    Serial.println("Failed to find NAU7802");
  }
  Serial.println("Found NAU7802");

  nau.setLDO(NAU7802_3V0);
  Serial.print("LDO voltage set to ");
  switch (nau.getLDO()) {
    case NAU7802_4V5:  Serial.println("4.5V"); break;
    case NAU7802_4V2:  Serial.println("4.2V"); break;
    case NAU7802_3V9:  Serial.println("3.9V"); break;
    case NAU7802_3V6:  Serial.println("3.6V"); break;
    case NAU7802_3V3:  Serial.println("3.3V"); break;
    case NAU7802_3V0:  Serial.println("3.0V"); break;
    case NAU7802_2V7:  Serial.println("2.7V"); break;
    case NAU7802_2V4:  Serial.println("2.4V"); break;
    case NAU7802_EXTERNAL:  Serial.println("External"); break;
  }

  nau.setGain(NAU7802_GAIN_128);
  Serial.print("Gain set to ");
  switch (nau.getGain()) {
    case NAU7802_GAIN_1:  Serial.println("1x"); break;
    case NAU7802_GAIN_2:  Serial.println("2x"); break;
    case NAU7802_GAIN_4:  Serial.println("4x"); break;
    case NAU7802_GAIN_8:  Serial.println("8x"); break;
    case NAU7802_GAIN_16:  Serial.println("16x"); break;
    case NAU7802_GAIN_32:  Serial.println("32x"); break;
    case NAU7802_GAIN_64:  Serial.println("64x"); break;
    case NAU7802_GAIN_128:  Serial.println("128x"); break;
  }

  nau.setRate(NAU7802_RATE_10SPS);
  Serial.print("Conversion rate set to ");
  switch (nau.getRate()) {
    case NAU7802_RATE_10SPS:  Serial.println("10 SPS"); break;
    case NAU7802_RATE_20SPS:  Serial.println("20 SPS"); break;
    case NAU7802_RATE_40SPS:  Serial.println("40 SPS"); break;
    case NAU7802_RATE_80SPS:  Serial.println("80 SPS"); break;
    case NAU7802_RATE_320SPS:  Serial.println("320 SPS"); break;
  }

  //setup LCD
  lcd.init();                
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(2,0);
  lcd.print("Setup is Complete");
  lcd.setCursor(4,1);

  //turn on lock
  digitalWrite(39, HIGH);
  for (int i = 25; i > 0; i--) {
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Urodynamic Testing");
    lcd.setCursor(3,1);
    lcd.print("Will Begin In:"); 
    lcd.setCursor(5,2);
    String s = String(i) + " Second(s)";
    lcd.print(s); 
    lcd.setCursor(1,3);
    lcd.print("Insert Catheter Now");
    delay(1000);
  }
  
  // Take 10 readings to flush out readings
  for (uint8_t i=0; i<10; i++) {
    while (! nau.available()) delay(1);
    nau.read();
  }

  while (! nau.calibrate(NAU7802_CALMOD_INTERNAL)) {
    Serial.println("Failed to calibrate internal offset, retrying!");
    delay(1000);
  }
  Serial.println("Calibrated internal offset");

  while (! nau.calibrate(NAU7802_CALMOD_OFFSET)) {
    Serial.println("Failed to calibrate system offset, retrying!");
    delay(1000);
  }
  Serial.println("Calibrated system offset");
  
  //setup TOF sensor
  while (! Serial) {
    delay(1);
  }
  Serial.println("Adafruit VL53L0X test.");
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  Serial.println(F("VL53L0X API Continuous Ranging example\n\n"));
  lox.startRangeContinuous(); // start continuous ranging
  
  
  for (int i = 30; i > 0; i--) {
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Adjust Height Now");
    lcd.setCursor(1,1);
    lcd.print("Measurements Start"); 
    lcd.setCursor(5,2);
    String s = String(i) + " Second(s)";
    lcd.print(s); 
    delay(1000);
  }

  digitalWrite(39, HIGH);
  delay(2000);

}



void loop() {
  //take volume measurement / pressure measurement
  int sampleTime = 30;
  int drainTime = 30;
  
  int sumVolume = 0;
  int oneVolume = 0;
  int32_t sumPressure = 0;
  int32_t onePressure = 0;
  
  for (int i = 0; i < sampleTime; i++) {
    oneVolume = calculateVolume();
    sumVolume = sumVolume + oneVolume;
    
    if (i > sampleTime - 5) {
      onePressure = calculatePressure();
      sumPressure = sumPressure + onePressure;
    }
    
    int timeLeft = sampleTime-i;
    updateDisplayCollectingState(timeLeft);
    delay(1000);
  }
  
  vol = sumVolume/sampleTime;
  if (vol < 0) {
    vol = 0;
  }

  //should possibly remove.
  pres = sumPressure/5;
  if (pres < 0) {
    pres = 0;
  }

  digitalWrite(39, LOW);
  
  for (int j = drainTime; j > 0; j--) {
   updateDisplayDrainState(vol,pres, j); 
   delay(1000);
  }
  
  digitalWrite(39, HIGH);
  
}

void dummyMainLoop() {
  //take volume measurement / pressure measurement
  int sampleTime = 30;
  int drainTime = 30;
  
  int sumVolume = 0;
  int oneVolume = 0;
  int32_t sumPressure = 0;
  int32_t onePressure = 0;
  
  for (int i = 0; i < sampleTime; i++) {
    oneVolume = calculateVolume();
    sumVolume = sumVolume + oneVolume;
    
    if (i > sampleTime - 5) {
      onePressure = calculatePressure();
      sumPressure = sumPressure + onePressure;
    }
    
    int timeLeft = sampleTime-i;
    updateDisplayCollectingState(timeLeft);
    delay(1000);
  }
  
  vol = sumVolume/sampleTime;
  if (vol < 0) {
    vol = 0;
  }

  //should possibly remove.
  pres = sumPressure/5;
  if (pres < 0) {
    pres = 0;
  }

  digitalWrite(39, LOW);
  
  for (int j = drainTime; j > 0; j--) {
   updateDisplayDrainState(vol,pres, j); 
   delay(1000);
  }
  
  digitalWrite(39, HIGH);
}

void dummyMotor() {
  delay(5000);
  runMotor(true, true);
  delay(15000);
  runMotor(false, true);
}

//If "state" is passes as TRUE, the locking mechanism is locked
//If "state" is passes as FALSE, the locking mechanism is unlocked
void triggerLock(bool state) {
  if (state) {
    digitalWrite(lockPin, HIGH);  // Set pin lockPin HIGH if state is true
    Serial.println("Lock is Locked");
  } else {
    digitalWrite(lockPin, LOW);   // Set pin lockPin LOW if state is false
    Serial.println("Lock is Unlocked");
  }
}

//If "state" is true and "polarity" is true: This turns the motor ON and will be forward (into the collection cup)
//If "state" is true and "polarity" is false: This turns the motor ON and will be reverse (out of the collection cup)
//If "state" is false and "polarity" is true: This turns the motor OFF and will be forward (into the collection cup)
//If "state" is false and "polarity" is false: This turns the motor OFF and will be reverse (out of the collection cup)
void runMotor(bool state, bool polarity) {
  if (state) {
    if(polarity) {
      //here the motor should be on and should be forward (into the collection cup)
      digitalWrite(MOTOR_IN1, LOW);
      for (int i=0; i<255; i++) {
        analogWrite(MOTOR_IN2, i);
        delay(10);
      }
    } else {
      //here the motor should be on and should be reverse (out of the collection cup) 
      digitalWrite(MOTOR_IN2, LOW);
      for (int i=0; i<255; i++) {
        analogWrite(MOTOR_IN1, i);
        delay(10);
      }
    }
  } else { 
    if(polarity) {
      //here the motor should be turining off and should be previosuly forward (into the collection cup)
      for (int i=255; i>=0; i--) {
        analogWrite(MOTOR_IN2, i);
        delay(10);
      }
    } else {
      //here the motor should be turning off and should be previosuly reverse (out of the collection cup) 
      for (int i=255; i>=0; i--) {
        analogWrite(MOTOR_IN1, i);
        delay(10);
      }
    }
  }
}

//reads the raw pressure sensor data to be sent to the calculatePressure() method for a readable value
int32_t readPressureSensor() {
  int32_t val = 0;
  if (nau.available()) {
    val = nau.read();
    //Serial.print("Read Pressure Sensor");
  } else {
    Serial.print("Pressure Sensor CANNOT BE READ");
  }
  return val;
}

int32_t calculatePressure() {
  return (0.000130337*readPressureSensor()) + (0.1627) + (2);
}

int readTOFSensor() {
  int val = 0;
  if (lox.isRangeComplete()) {
    val = lox.readRange();
    //Serial.print("Read TOF Sensor");
  } else {
    Serial.print("TOF Sensor CANNOT BE READ");
  }
  return val;
}

int calculateVolume() {
  int rawVal = readTOFSensor();
  int volumeVal = ((-3.2872 + 0.35)*rawVal)+ (496.4);

  if (volumeVal > 225) {
    volumeVal = volumeVal + 10;
  }

  if (volumeVal > 425) {
    volumeVal = volumeVal + 10;
  }

  return volumeVal;
}

void updateDisplayDrainState(int vol, int32_t pressure, int secondsLeft) {
  lcd.clear();
  
  lcd.setCursor(0,0);
  String s1 = "Last P Val: " + String(pressure) + " cmH2O";
  lcd.print(s1);
  
  lcd.setCursor(0,1);
  String s2 = "Last V Val: " + String(vol) + " ml";
  lcd.print(s2);
  

  lcd.setCursor(3,2);
  lcd.print("Draining for:");

  lcd.setCursor(4,3);
  String s3 = String(secondsLeft) + " Second(s)";
  lcd.print(s3);
}


void updateDisplayCollectingState(int secondsLeft) {
  lcd.clear();
  
  lcd.setCursor(1,0);
  lcd.print("Bladder is Locked");
  
  lcd.setCursor(2,1);
  lcd.print("Data Collection"); 
  
  lcd.setCursor(3,2);
  lcd.print("Concludes in: ");
   
  lcd.setCursor(4,3);
  String s = String(secondsLeft) + " Second(s)";
  lcd.print(s); 
}
