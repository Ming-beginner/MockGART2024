//Main code for team RED's bot for Mock GART 2024
// For controlling PCA9685
#include <Wire.h>
// For using PS2 controller
#include <PS2X_lib.h>

// PWM mapping
const int left1 = 27; //27
const int left2 = 14; //14
const int right1 = 13; 
const int right2 = 12;
const int intake1 = 16; //16
const int intake2 = 17; //17
const int outtake1 = 18;
const int outtake2 = 5;
const int hatch = 2;


const int HOLD_HATCH = 134; //Adjust these value to suit the servo
const int CLOSE_HATCH = 320;
const int OUTTAKE_SPEED = 4095;
const int NORMAL_SPEED = 22;
const int MAX_SPEED = 31;

/******************************************************************
 * Pins config for the library :
 * - On the motorshield of VIA Makerbot BANHMI, there is a 6-pin
 *   header designed for connecting PS2 remote controller.
 * Header pins and corresponding GPIO pins:
 *   MOSI | MISO | GND | 3.3V | CS | CLK
 *    12     13    GND   3.3V   15   14
 ******************************************************************/

#define PS2_DAT 26 // MISO
#define PS2_CMD 25 // MOSI
#define PS2_SEL 33 // SS
#define PS2_CLK 32 // SLK

/******************************************************************
 * Select mode for PS2 controller:
 *   - pressures = Read analog value from buttons
 *   - rumble    = Turn on / off rumble mode
 ******************************************************************/
#define pressures false
#define rumble false

class pwm_t
{
  public:
  static void setPWM(int port, int ignored, double val)
  {
    analogWrite(port, (int)((val*256)/4096));
    Serial.print(port);
    Serial.print(' ');
    Serial.println(val);
  }
};
pwm_t pwm;
PS2X ps2x; // Create ps2x instance

// Control mode
int bco = 22;

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
    if(ps2x.Button(PSB_L2)) {
      bco = MAX_SPEED;
    } else {
      bco = NORMAL_SPEED;
    }
  }else if(ps2x.Button(PSB_L2)) {
    pwm.setPWM(intake1, 0, 0);
    pwm.setPWM(intake2, 0, 4095);
  } else {                                                                                        
    pwm.setPWM(intake1, 0, 0);
    pwm.setPWM(intake2, 0, 0);
  }
}

void handleOuttake() {
  if (ps2x.Button(PSB_R1)) {
    pwm.setPWM(outtake1, 0, OUTTAKE_SPEED);
    pwm.setPWM(outtake2, 0, 0);
  } else if(ps2x.Button(PSB_R2)) {
    pwm.setPWM(outtake1, 0, 0);
    pwm.setPWM(outtake2, 0, OUTTAKE_SPEED);
  } else {                                                                                        
    pwm.setPWM(outtake1, 0, 0);
    pwm.setPWM(outtake2, 0, 0);
  }
}



bool hatchState = 0;

void handleHatch() {
  // if(ps2x.ButtonPressed(PSB_CIRCLE)) {
  //   if(pwm.getPWM(hatch, 1) != HOLD_HATCH) {
  //     pwm.setPWM(hatch, 0, HOLD_HATCH);
  //   } else {
  //     pwm.setPWM(hatch, 0, CLOSE_HATCH);
  //   }
  // }
}


void setup() {
  // Connect to PS2 
  int arr[] = {13, 12, 14, 27, 5, 18, 16, 17, 35};
  for (int a : arr)
  {
    pinMode(a, OUTPUT);
    digitalWrite(a, LOW);
  }
  Serial.begin(115200);
  Serial.println("Connecting to gamepad");
  int error = -1;
  //while(error != 0) {
    for (int i = 0; i < 10; i++) // thử kết nối với tay cầm ps2 trong 10 lần
    {
      delay(1000); // đợi 1 giây
      // cài đặt chân và các chế độ: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
      error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);
      Serial.print(".");
      if (error == 0) break;
    }
  //}

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
  Wire.setClock(400000); // Set to max i2c frequency @ 400000
}


void loop() {
  // Read the gamepad state
  ps2x.read_gamepad(false, false);
  // Serial.print("Prev: ");
  // Serial.println(prevState);
  bool curr = ps2x.ButtonPressed(PSB_SQUARE);
  handleIntake();
  handleOuttake();
  handleHatch();
  if(curr) {
    isArcadeMode = !isArcadeMode;
  }
  //Choose between 2 driving mode
  if(isArcadeMode) {
    arcadeDrive();
  } else {
    tankDrive();
  }
  //Delay a little bit
  delay(10);
}