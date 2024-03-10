//Main code for team RED's bot for Mock GART 2024

// For controlling PCA9685
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
// For using PS2 controller
#include <PS2X_lib.h>


// PWM mapping
const int left1 = 8;
const int left2 = 9;
const int right1 = 14;
const int right2 = 15;
const int intake1 = 10;
const int intake2 = 11;
const int outtake1 = 12;
const int outtake2 = 13;

/******************************************************************
 * Pins config for the library :
 * - On the motorshield of VIA Makerbot BANHMI, there is a 6-pin
 *   header designed for connecting PS2 remote controller.
 * Header pins and corresponding GPIO pins:
 *   MOSI | MISO | GND | 3.3V | CS | CLK
 *    12     13    GND   3.3V   15   14
 ******************************************************************/

#define PS2_DAT 12 // MISO
#define PS2_CMD 13 // MOSI
#define PS2_SEL 15 // SS
#define PS2_CLK 14 // SLK

/******************************************************************
 * Select mode for PS2 controller:
 *   - pressures = Read analog value from buttons
 *   - rumble    = Turn on / off rumble mode
 ******************************************************************/
#define pressures false
#define rumble false

PS2X ps2x; // Create ps2x instance
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(); // Create pwm instance

// Control mode
int bco = 19;
string driveMode = "arcadedrive";

void setLeftMotorVelocity(int velocity) {
  if(velocity >= 0) {
    pwm.setPWM(left1, 0, velocity);
    pwm.setPWM(left2, 0, 0);
  } else {
    pwm.setPWM(left1, 0, 0);
    pwm.setPWM(left2, 0, -velocity);
  }
}

void setRightMotorVelocity(int velocity) {
  if(velocity >= 0) {
    pwm.setPWM(right2, 0, velocity);
    pwm.setPWM(right1, 0, 0);
  } else {
    pwm.setPWM(right2, 0, 0);
    pwm.setPWM(right1, 0, -velocity);
  }
}

void tankDrive() {
  //Left wheel control
  int leftJoystickValue = ps2x.Analog(PSS_LY);
  int leftVelocity = bco*(128 - leftJoystickValue);
  setLeftMotorVelocity(leftVelocity);
  // Right wheel control
  int rightJoystickValue = ps2x.Analog(PSS_RY);
  int rightVelocity = bco*abs(128 - rightJoystickValue);
  setRightMotorVelocity(rightVelocity);
}


void arcadeDrive() {
  int hosizontalJoystickValue = ps2x.Analog(PSS_RX);
  int verticalJoystickVal = ps2x.Analog(PSS_LY);
  int drive = 128-verticalJoystickVal;
  int rotate = hosizontalJoystickValue-128;
  int maximum = max(abs(drive), abs(rotate));
  int total = drive + rotate;
  int diff = drive - rotate;
  if(drive > 0) {
    if(rotate < 0) {
      //Left motor
      setLeftMotorVelocity(total);
      //Right motor
      setRightMotorVelocity(maximum);
    } else {
      //Left motor
      setLeftMotorVelocity(maximum);
      //Right motor
      setRightMotorVelocity(diff);
    }
  } else {
    if(rotate < 0) {
      //Left motor
      setLeftMotorVelocity(-maximum);
      //Right motor
      setRightMotorVelocity(diff);
    } else {
      //Left motor
      setLeftMotorVelocity(total);
      //Right motor
      setRightMotorVelocity(-maximum);
    }
  }

}


void handleIntake() {
  if (ps2x.Button(PSB_L1)) {
    if (ps2x.Button(PSB_L2)) {
      pwm.setPWM(intake1, 0, 4095);
      pwm.setPWM(intake2, 0, 0);
    } else {
      pwm.setPWM(intake1, 0, 0);
      pwm.setPWM(intake2, 0, 4095);
    }
  } else {                                                                                        
    pwm.setPWM(intake1, 0, 0);
    pwm.setPWM(intake2, 0, 0);
  }
}

void handleDriveMode() {
  if(ps2x.NewButtonState(PSB_SQUARE)) {
    driveMode = driveMode == "tankdrive" ? "arcadedrive" : "tankdrive";
  }
}

void handleBoostMode() {
  if (ps2x.NewButtonState(PSB_TRIANGLE)) {
    if (ps2x.Button(PSB_TRIANGLE)) {
      // Boost mode
      bco = 29;
    } else {
      bco = 19;
    }
  }
}

void setup() {

  // Connect to PS2 
  Serial.print("Connecting to gamepad");

  int error = -1;
  bool connecting = true;
  while(connecting) { //Connecting to gamepad
    delay(1000); // wait a second before
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
    Serial.print(".");
    if (error == 0) {
      connecting = false;
      Serial.println("Connected to gamepad successfully");
    }
    else {
      Serial.println("Failed to connect to gamepad...");
    }
  }
  // Init motor controller
  pwm.begin(); // Initialize PCA9685 
  pwm.setOscillatorFrequency(27000000); // Set frequency for PCA9685
  pwm.setPWMFreq(50); // PWM frequency. Should be 50-60 Hz for controlling both DC motor and servo
  Wire.setClock(400000); // Set to max i2c frequency @ 400000
}



void loop() {
  // Read the gamepad state
  ps2x.read_gamepad(false, false);
  handleBoostMode();
  //Choose between 2 driving mode
  handleDriveMode();
  if(mode == "tankdrive") {
    tankDrive();
  } else {
    arcadeDrvie();
  }
  handleIntake();

  //Outtake
  // Put some new code here
  //Delay a little bit
  delay(10);
}