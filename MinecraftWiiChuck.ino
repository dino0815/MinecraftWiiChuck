/*
 * MinecraftWiiChuck
 *
 * 2013 miznash
 *
 * Let's play minecraft with Arduino Leonard + Wii nunchuck!
 * ------------------------------------------------------------------
 * THE SOFTWARE IS PROVIDED ÅgAS ISÅh, WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED.
 * ------------------------------------------------------------------
 *
 *  <Keyboard operation>
 *  A,W,S,D ... Joystick
 *  Space   ... Z
 *  E       ... C + Z
 *  Q       ... C + Shake Wii Nunchuck
 *
 *  <Mouse operation>
 *  Move        ... Wii Nunchuck roll & pitch
 *  Left click  ... Shake Wii Nunchuck
 *  Right click ... C (When C button is released) 
 *  Mouse wheel ... C + Roll joystick
 *  
 *  <Get / Release Mouse & Keyboard> 
 *  C + Z + Joystick down
 *  (You need to do this operation first when this program is started.)
 *
 */

#include <Wire.h>
#include "WiiChuck.h"

typedef boolean buttonState;
#define BUTTON_PRESSED   true
#define BUTTON_RELEASED  false

#define MAIN_LOOP_INTERVAL_MSEC    30
#define THRESH_SHAKE               50
#define SHAKE_SENSITIVITY           5
#define THRESH_MOUSE_MOVE          16
#define CONTROL_ON_OFF_SENSITIVITY 10 // x MAIN_LOOP_INTERVAL_MSEC is needed for on/off control

#define JOY_PRESS_THRESH_SOFT   30
#define JOY_PRESS_THRESH_NORMAL 60

WiiChuck chuck = WiiChuck();

boolean isJumping = false;
boolean skipCbuttonReleased = false;

JOY_POS lastJoyPosLR = JOY_POS_NEUTRAL;
JOY_POS joyPosLR     = JOY_POS_NEUTRAL;
JOY_POS lastJoyPosUD = JOY_POS_NEUTRAL;
JOY_POS joyPosUD     = JOY_POS_NEUTRAL;
boolean joyPressSoft = false;

int lastJoyPosForWheel = -1;
int joyPosForWheel = -1;

int lpfAccelX = 0;
int lpfAccelY = 0;
int lpfAccelZ = 0;
int lastAccelX = 0;
int lastAccelY = 0;
int lastAccelZ = 0;

void setup()
{
  Serial.begin(19200);
  while (!Serial) { 
    ; 
  }  // Waiting serial ready (for Arduino Leonard)
  chuck.begin();
  chuck.update();
  Mouse.begin();
  Keyboard.begin();
}

void onCbuttonPressed()
{
  Serial.println("C pressed");
}

void onCbuttonReleased()
{
  if (!skipCbuttonReleased) {
    Mouse.press(MOUSE_RIGHT);
    Mouse.release(MOUSE_RIGHT);
    Serial.println("C released");
  }
  else {
    skipCbuttonReleased = false;
  }
}

void onZbuttonPressed()
{
  if (!isJumping) {
    Keyboard.press(' ');
    isJumping = true; 
  }
}

void onZbuttonReleased()
{
  if (isJumping) {   
    Keyboard.release(' ');
    isJumping = false;
  }
}

void onCZbuttonsPressed()
{
  Keyboard.press('e');
  Keyboard.release('e');
  Serial.println("C and Z pressed");
  skipCbuttonReleased = true;
}

void mouseEmulation()
{
  static int shakeSensitivityCounter = 0;
  int mouseMoveStepLR = 0;
  int mouseMoveStepUD = 0;

  if (abs(lpfAccelZ - chuck.readAccelZ()) > THRESH_SHAKE) {
    if (chuck.isCbuttonPressed()) {
      // C button press + shake = 'Q' throw item
      Keyboard.press('q');
      Keyboard.release('q');
      skipCbuttonReleased = true;
    }
    else {
      Mouse.press(MOUSE_LEFT);
      shakeSensitivityCounter = SHAKE_SENSITIVITY;
    }
  }
  else {
    if (shakeSensitivityCounter > 0) {
      shakeSensitivityCounter--;
    }
    if (shakeSensitivityCounter == 0) {
      Mouse.release(MOUSE_LEFT);
    }
    // Change player's sight (Mouse move)
    if (!chuck.isZbuttonPressed() && shakeSensitivityCounter == 0) {
      mouseMoveStepLR = chuck.readRoll();
      mouseMoveStepLR = (abs(mouseMoveStepLR) > THRESH_MOUSE_MOVE ? (mouseMoveStepLR >> 2) : 0);
      mouseMoveStepUD = -(chuck.readPitch() - 90);  // Pitch 90 degrees = even with the ground
      mouseMoveStepUD = (abs(mouseMoveStepUD) > THRESH_MOUSE_MOVE ? (mouseMoveStepUD >> 2) : 0);
      Mouse.move(mouseMoveStepLR, mouseMoveStepUD, 0);
    }
  }

  // Mouse Wheel = Roll JoyStick
  mouseWheelEmulation();
}

void mouseWheelEmulation()
{
  // The direction of joystick correspond to the index number.
  // If last position is 0 and current position is 2,
  // wheelStepTable[0][2] = 2. It means wheeling mouse 2 steps.
  //
  //        2
  //     1  |  3
  //   0  --+--  4
  //     7  |  5
  //        6
  static signed char wheelStepTable[8][8] = { // [last][current]
    0,  1,  2,  3,  0, -3, -2, -1,
    -1,  0,  1,  2,  3,  0, -3, -2,
    -2, -1,  0,  1,  2,  3,  0, -3,
    -3, -2, -1,  0,  1,  2,  3,  0,
    0, -3, -2, -1,  0,  1,  2,  3,
    3,  0, -3, -2, -1,  0,  1,  2,
    2,  3,  0, -3, -2, -1,  0,  1,
    1,  2,  3,  0, -3, -2, -1,  0
  };

  signed char wheelStep;
  if (chuck.isCbuttonPressed()) {
    if (joyPosForWheel != -1) {
      skipCbuttonReleased = true;
      if (lastJoyPosForWheel != -1) {
        wheelStep = wheelStepTable[lastJoyPosForWheel][joyPosForWheel];
        Mouse.move(0, 0, wheelStep);
      }
    }
  }
}

void updateJoyPosForWheel()
{
  lastJoyPosForWheel = joyPosForWheel;

  switch (chuck.getJoyPosLR()) {
  case JOY_POS_NEUTRAL:
    switch (chuck.getJoyPosUD()) {
    case JOY_POS_NEUTRAL:
      joyPosForWheel = -1;
      break;
    case JOY_POS_UP:
      joyPosForWheel = 2;
      break;
    case JOY_POS_DOWN:
      joyPosForWheel = 6;
      break;
    default:
      ;
    }
    break;
  case JOY_POS_LEFT:
    switch (chuck.getJoyPosUD()) {
    case JOY_POS_NEUTRAL:
      joyPosForWheel = -1;
      break;
    case JOY_POS_UP:
      joyPosForWheel = 1;
      break;
    case JOY_POS_DOWN:
      joyPosForWheel = 7;
      break;
    default:
      ;
    }
    break;
  case JOY_POS_RIGHT:
    switch (chuck.getJoyPosUD()) {
    case JOY_POS_NEUTRAL:
      joyPosForWheel = 4;
      break;
    case JOY_POS_UP:
      joyPosForWheel = 3;
      break;
    case JOY_POS_DOWN:
      joyPosForWheel = 5;
      break;
    default:
      break;
    }
    break;
  default:
    ;
  } 
}

void onJoyPosNeutral()
{
  Serial.println("onJoyPosNeutral");
  switch (lastJoyPosLR) {
  case JOY_POS_LEFT:
    Keyboard.release('a');
    break;
  case JOY_POS_RIGHT:
    Keyboard.release('d');
    break;
  case JOY_POS_NEUTRAL:
    break;
  default:
    ;
  }
  switch (lastJoyPosUD) {
  case JOY_POS_UP:
    Keyboard.release('w');
    break;
  case JOY_POS_DOWN:
    Keyboard.release('s');
    break;
  case JOY_POS_NEUTRAL:
    break;
  default:
    ;
  }

}

void onJoyPosUp()
{
  switch (lastJoyPosUD) {
  case JOY_POS_UP:
    break;
  case JOY_POS_DOWN:
    Keyboard.release('s');
    Keyboard.press('w');
    break;
  case JOY_POS_NEUTRAL:
    Keyboard.press('w');
    break;
  default:
    ;
  }
  Serial.println("onJoyPosUp");
}

void onJoyPosDown()
{
  switch (lastJoyPosUD) {
  case JOY_POS_UP:
    Keyboard.release('w');
    Keyboard.press('s');
    break;
  case JOY_POS_DOWN:
    break;
  case JOY_POS_NEUTRAL:
    Keyboard.press('s');
    break;
  default:
    ;
  }
  Serial.println("onJoyPosDown");
}

void onJoyPosRight()
{
  Serial.println("onJoyPosRight");
  switch (lastJoyPosLR) {
  case JOY_POS_LEFT:
    Keyboard.release('a');
    Keyboard.press('d');
    break;
  case JOY_POS_RIGHT:
    break;
  case JOY_POS_NEUTRAL:
    Keyboard.press('d');
    break;
  default:
    ;
  }
}

void onJoyPosLeft()
{
  Serial.println("onJoyPosLeft");
  switch (lastJoyPosLR) {
  case JOY_POS_LEFT:
    break;
  case JOY_POS_RIGHT:
    Keyboard.release('d');
    Keyboard.press('a');
    break;
  case JOY_POS_NEUTRAL:
    Keyboard.press('a');
    break;
  default:
    ;
  }
}


void fireButtonEvent()
{
  static buttonState lastCbutton = BUTTON_RELEASED;
  static buttonState lastZbutton = BUTTON_RELEASED;
  static buttonState cButton = BUTTON_RELEASED;
  static buttonState zButton = BUTTON_RELEASED;

  lastCbutton = cButton;
  cButton = (chuck.isCbuttonPressed() ? BUTTON_PRESSED : BUTTON_RELEASED);
  lastZbutton = zButton;
  zButton = (chuck.isZbuttonPressed() ? BUTTON_PRESSED : BUTTON_RELEASED);

  // fire event
  if (lastCbutton == BUTTON_RELEASED && cButton == BUTTON_PRESSED) {
    onCbuttonPressed();
  }
  if (lastCbutton == BUTTON_PRESSED && cButton == BUTTON_RELEASED) {
    onCbuttonReleased();
  }
  if (lastZbutton == BUTTON_RELEASED && zButton == BUTTON_PRESSED) {
    onZbuttonPressed();
  }
  if (lastZbutton == BUTTON_PRESSED && zButton == BUTTON_RELEASED) {
    onZbuttonReleased();
  }
  if ((cButton == BUTTON_PRESSED && zButton == BUTTON_PRESSED)
    && (lastCbutton == BUTTON_RELEASED || lastZbutton == BUTTON_RELEASED)) {
    onCZbuttonsPressed();
  }
}

void fireJoystickEvent()
{
  boolean fireJoyPosNeutral = false;
  lastJoyPosUD = joyPosUD;
  lastJoyPosLR = joyPosLR;

  joyPressSoft = false;
  joyPosUD = chuck.getJoyPosUD(JOY_PRESS_THRESH_NORMAL);
  joyPosLR = chuck.getJoyPosLR(JOY_PRESS_THRESH_NORMAL);
  if (joyPosUD == JOY_POS_NEUTRAL
    && joyPosLR == JOY_POS_NEUTRAL) {
    joyPosUD = chuck.getJoyPosUD(JOY_PRESS_THRESH_SOFT);
    joyPosLR = chuck.getJoyPosLR(JOY_PRESS_THRESH_SOFT);
    if (joyPosUD != JOY_POS_NEUTRAL
      || joyPosLR != JOY_POS_NEUTRAL) {
      joyPressSoft = true;
    }
  }

  // To prevent from player's move when mouse wheeling,
  // player cannot move when C button is pressed.
  if (!chuck.isCbuttonPressed()) {
    if (joyPressSoft) {
      Keyboard.press(KEY_LEFT_SHIFT);
    }
    else {
      Keyboard.release(KEY_LEFT_SHIFT);
    }

    switch (joyPosUD) {
    case JOY_POS_NEUTRAL:
      if (joyPosUD != lastJoyPosUD) {
        fireJoyPosNeutral = true;
      }
      break;
    case JOY_POS_UP:
      if (joyPosUD != lastJoyPosUD) {
        onJoyPosUp();
      }
      break;
    case JOY_POS_DOWN:
      if (joyPosUD != lastJoyPosUD) {
        onJoyPosDown();
      }
      break;
    default:
      ;
    }
    switch (joyPosLR) {
    case JOY_POS_NEUTRAL:
      if (joyPosLR != lastJoyPosLR) {
        fireJoyPosNeutral = true;
      }
      break;
    case JOY_POS_LEFT:
      if (joyPosLR != lastJoyPosLR) {
        onJoyPosLeft();
      }
      break;
    case JOY_POS_RIGHT:
      if (joyPosLR != lastJoyPosLR) {
        onJoyPosRight();
      }
      break;
    default:
      ;
    }
    if (fireJoyPosNeutral) {
      onJoyPosNeutral();
    }
  }

  updateJoyPosForWheel();
}


void loop()
{
  static boolean isInControl = false;  // Mouse & Keyboard control from Arduino
  static int controlOnOffCounter = 0;
  static int loop_cnt = 0;

  if (loop_cnt > MAIN_LOOP_INTERVAL_MSEC) { // every 30 msecs get new data
    loop_cnt = 0;

    chuck.update(); 

    // for releasing mouse & keyboard control
    if (chuck.isZbuttonPressed() && chuck.isCbuttonPressed() && chuck.isJoyStickDown()) {
      controlOnOffCounter++;
      if (controlOnOffCounter > CONTROL_ON_OFF_SENSITIVITY) {
        if (isInControl) {
          Keyboard.releaseAll();
          Mouse.release(MOUSE_LEFT | MOUSE_RIGHT);
          Serial.print("Control end.");
        }
        else {
          Serial.print("Control start.");
        }
        isInControl = !isInControl;
        controlOnOffCounter = 0;
      }
    }
    else {
      controlOnOffCounter = 0;
    }


    if (isInControl) {
      fireButtonEvent();
      fireJoystickEvent();
      mouseEmulation();
    }

    lastAccelX = chuck.readAccelX();
    lastAccelY = chuck.readAccelY();
    lastAccelZ = chuck.readAccelZ();

    lpfAccelX = lastAccelX * 0.9 + (float)lpfAccelX * 0.1;
    lpfAccelY = lastAccelY * 0.9 + (float)lpfAccelY * 0.1;
    lpfAccelZ = lastAccelZ * 0.9 + (float)lpfAccelZ * 0.1;
  }
  loop_cnt++;
  delay(1);
}


