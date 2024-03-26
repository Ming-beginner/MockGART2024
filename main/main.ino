#include <PS2X_lib.h>

//Main code for team RED's bot for Mock GART 2024

// For controlling PCA9685
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
// For using PS2 controller
#include <PS2X_lib.h>


// PWM mapping
const int left1 = 9;
const int left2 = 8;
const int right1 = 14;
const int right2 = 15;
const int intake1 = 12;
const int intake2 = 13;
const int outtake1 = 10;
const int outtake2 = 11;
const int hatch = 2;


const int HOLD_HATCH = 134; //Adjust these value to suit the servo
const int HOLD_HATCH = 350;

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
int bco = 21;

bool isArcadeMode = 0;

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
  int rightVelocity = bco*(128 - rightJoystickValue);
  setRightMotorVelocity(rightVelocity);
}
int i=0;

void arcadeDrive() {
  int horizontalJoystickValue = ps2x.Analog(PSS_RX);
  int verticalJoystickValue = ps2x.Analog(PSS_LY);
  int drive = 128-verticalJoystickValue;
  int rotate = horizontalJoystickValue-127;
  #if DEBUG
  i++;
  if (i%1000 == 0)
  {Serial.print("RX: ");
  Serial.println(horizontalJoystickValue);
  Serial.print("LY: ");
  Serial.println(verticalJoystickValue);
  Serial.print("Drive: ");
  Serial.println(drive);
  Serial.print("Rotate: ");
  Serial.println(rotate);}
  #endif
  int maximum = max(abs(drive), abs(rotate));
  int total = drive + rotate;
  int diff = drive - rotate;
  maximum *= bco;
  total *= bco;
  diff *= bco;
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
    pwm.setPWM(intake1, 0, 4095);
    pwm.setPWM(intake2, 0, 0);
  } else if(ps2x.Button(PSB_L2)) {
    pwm.setPWM(intake1, 0, 0);
    pwm.setPWM(intake2, 0, 4095);
  } else {                                                                                        
    pwm.setPWM(intake1, 0, 0);
    pwm.setPWM(intake2, 0, 0);
  }
}

void handleOuttake() {
  if (ps2x.Button(PSB_R1)) {
    pwm.setPWM(outtake1, 0, 4095);
    pwm.setPWM(outtake2, 0, 0);
  } else if(ps2x.Button(PSB_R2)) {
    pwm.setPWM(outtake1, 0, 0);
    pwm.setPWM(outtake2, 0, 4095);
  } else {                                                                                        
    pwm.setPWM(outtake1, 0, 0);
    pwm.setPWM(outtake2, 0, 0);
  }
}



bool hatchState = 0;

void handleHatch() {
  if(ps2x.ButtonPressed(PSB_CIRCLE)) {
    if(pwm.getPWM(hatch, 1) != HOLD_HATCH) {
      pwm.setPWM(hatch, 0, HOLD_HATCH);
    } else {
      pwm.setPWM(hatch, 0, OPEN_HATCH);
    }
  }
}

void handleBoost() {
  if (ps2x.NewButtonState(PSB_TRIANGLE)) {
    if (ps2x.Button(PSB_TRIANGLE)) {
      // Boost mode
      bco = 30;
    } else {
      bco = 21;
    }
  }
}

void setup() {
  // Connect to PS2 
  delay(1000);
  Serial.begin(9600);
  Serial.println("Connecting to gamepad");
  int error = -1;
  for (int i = 0; i < 10; i++) // thử kết nối với tay cầm ps2 trong 10 lần
  {
    delay(1000); // đợi 1 giây
    // cài đặt chân và các chế độ: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
    error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
    Serial.print(".");
    if (error == 0) break;
  }

  switch (error) // kiểm tra lỗi nếu sau 10 lần không kết nối được
  {
  case 0:
    Serial.println(" Ket noi tay cam PS2 thanh cong");
    break;
  case 1:
    Serial.println(" LOI: Khong tim thay tay cam, hay kiem tra day ket noi vơi tay cam ");
    break;
  case 2:
    Serial.println(" LOI: khong gui duoc lenh");
    break;
  case 3:
    Serial.println(" LOI: Khong vao duoc Pressures mode ");
    break;
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
  // Serial.print("Prev: ");
  // Serial.println(prevState);
  bool curr = ps2x.ButtonPressed(PSB_SQUARE);
  if(curr) {
    isArcadeMode = !isArcadeMode;
  }
  //Choose between 2 driving mode
  if(isArcadeMode) {
    arcadeDrive();
  } else {
    tankDrive();
  }
  handleIntake();
  handleOuttake();
  handleHatch();
  handleBoost();
  //Delay a little bit
  delay(10);
}