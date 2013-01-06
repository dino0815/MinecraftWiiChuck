/*
 * WiiChuck class for playing Minecraft
 * 2013 miznash
 *
 * The original code is Tim Hirzel's WiiChuck class and it's an adaptation of
 * the code by Tod E. Kurt.
 * http://playground.arduino.cc/Main/WiiChuckClass
 *
 * ------------------------------------------------------------------
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED.
 * ------------------------------------------------------------------
 *
 *
 * Nunchuck -- Use a Wii Nunchuck
 * Tim Hirzel http://www.growdown.com
 * 
 notes on Wii Nunchuck Behavior.
 This library provides an improved derivation of rotation angles from the nunchuck accelerometer data.
 The biggest different over existing libraries (that I know of ) is the full 360 degrees of Roll data
 from teh combination of the x and z axis accelerometer data using the math library atan2. 
 
 It is accurate with 360 degrees of roll (rotation around axis coming out of the c button, the front of the wii),
 and about 180 degrees of pitch (rotation about the axis coming out of the side of the wii).  (read more below)
 
 In terms of mapping the wii position to angles, its important to note that while the Nunchuck
 sense Pitch, and Roll, it does not sense Yaw, or the compass direction.  This creates an important
 disparity where the nunchuck only works within one hemisphere.  At a result, when the pitch values are 
 less than about 10, and greater than about 170, the Roll data gets very unstable.  essentially, the roll
 data flips over 180 degrees very quickly.   To understand this property better, rotate the wii around the
 axis of the joystick.  You see the sensor data stays constant (with noise).  Because of this, it cant know
 the difference between arriving upside via 180 degree Roll, or 180 degree pitch.  It just assumes its always
 180 roll.
 
 
 * 
 * This file is an adaptation of the code by these authors:
 * Tod E. Kurt, http://todbot.com/blog/
 *
 * The Wii Nunchuck reading code is taken from Windmeadow Labs
 * http://www.windmeadow.com/node/42
 */

#ifndef WiiChuck_h
#define WiiChuck_h

#if (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <Wire.h>
#include <math.h>


// these may need to be adjusted for each nunchuck for calibration
#define ZEROX 510  
#define ZEROY 490
//#define ZEROZ 720
#define ZEROZ 460
#define RADIUS 210  // probably pretty universal

#define DEFAULT_ZERO_JOY_X 124
#define DEFAULT_ZERO_JOY_Y 132

enum JOY_POS {
    JOY_POS_NEUTRAL = 0,
    JOY_POS_LEFT = 1,
    JOY_POS_UP = 1,
    JOY_POS_RIGHT = -1,
    JOY_POS_DOWN = -1
};

class WiiChuck {
public:

private:
  byte cnt;
  uint8_t status[6];		// array to store wiichuck output
  byte averageCounter;
  //int accelArray[3][AVERAGE_N];  // X,Y,Z
  int i;
  int total;
  uint8_t zeroJoyX;   // these are about where mine are
  uint8_t zeroJoyY; // use calibrateJoy when the stick is at zero to correct
  int lastJoyX;
  int lastJoyY;
  int angles[3];

  boolean lastZ, lastC;


public:

  byte joyX;
  byte joyY;
  boolean buttonZ;
  boolean buttonC;
  void begin() 
  {
    Wire.begin();
    cnt = 0;
    averageCounter = 0;
    Wire.begin();                // join i2c bus as master
    Wire.beginTransmission(0x52);// transmit to device 0x52
#if (ARDUINO >= 100)
    Wire.write((uint8_t)0x40);// sends memory address
    Wire.write((uint8_t)0x00);// sends sent a zero.  
#else
    Wire.send((uint8_t)0x40);// sends memory address
    Wire.send((uint8_t)0x00);// sends sent a zero.  
#endif
    Wire.endTransmission();// stop transmitting
    update();            
    for (i = 0; i<3;i++) {
      angles[i] = 0;
    }
    zeroJoyX = DEFAULT_ZERO_JOY_X;
    zeroJoyY = DEFAULT_ZERO_JOY_Y;
  }


  void calibrateJoy() {
    zeroJoyX = joyX;
    zeroJoyY = joyY;
  }

  void update() {

    Wire.requestFrom (0x52, 6);	// request data from nunchuck
    while (Wire.available ()) {
      // receive byte as an integer
#if (ARDUINO >= 100)
      status[cnt] = _nunchuk_decode_byte( Wire.read() );
#else
      status[cnt] = _nunchuk_decode_byte( Wire.receive() );
#endif
      cnt++;
    }
    if (cnt > 5) {
      lastZ = buttonZ;
      lastC = buttonC;
      lastJoyX = readJoyX();
      lastJoyY = readJoyY();

      cnt = 0;
      joyX = (status[0]);
      joyY = (status[1]);
      for (i = 0; i < 3; i++) {
        //angles[i] = (status[i+2] << 2) + ((status[5] & (B00000011 << ((i+1)*2) ) >> ((i+1)*2))); 
        angles[i] = (status[i+2] << 2) + (((status[5] & (B00000011 << ((i+1)*2) )) >> ((i+1)*2))); 
      }

      buttonZ = !( status[5] & B00000001);
      buttonC = !((status[5] & B00000010) >> 1);
      _send_zero(); // send the request for next bytes
    }
  }

  float readAccelX() {
    // total = 0; // accelArray[xyz][averageCounter] * FAST_WEIGHT;
    return (float)angles[0] - ZEROX;
  }
  float readAccelY() {
    // total = 0; // accelArray[xyz][averageCounter] * FAST_WEIGHT;
    return (float)angles[1] - ZEROY;
  }
  float readAccelZ() {
    // total = 0; // accelArray[xyz][averageCounter] * FAST_WEIGHT;
    return (float)angles[2] - ZEROZ;
  }

  boolean zPressed() {
    return (buttonZ && ! lastZ);
  }
  boolean isCbuttonPressed() {
    return buttonC;
  }
  boolean isZbuttonPressed() {
    return buttonZ;    
  }

  boolean isJoyPressed(int thresh=60) {
    return (isJoyStickUp(thresh) | isJoyStickDown(thresh) | isJoyStickLeft(thresh) | isJoyStickRight(thresh));
  }
  // for using the joystick like a directional button
  boolean isJoyStickRight(int thresh=60) {
    return (readJoyX() > thresh);
  }

  // for using the joystick like a directional button
  boolean isJoyStickLeft(int thresh=60) {
    return (readJoyX() < -thresh);
  }

  // for using the joystick like a directional button
  boolean isJoyStickUp(int thresh=60) {
    return (readJoyY() > thresh);
  }

  // for using the joystick like a directional button
  boolean isJoyStickDown(int thresh=60) {
    return (readJoyY() < -thresh);
  }

  JOY_POS getJoyPosLR(int thresh = 60) {
    if (isJoyStickLeft(thresh)) {
      return JOY_POS_LEFT;
    }
    else if (isJoyStickRight(thresh)) {
      return JOY_POS_RIGHT;      
    }
    else {
      return JOY_POS_NEUTRAL;
    }
  }

  JOY_POS getJoyPosUD(int thresh = 60) {
    if (isJoyStickUp(thresh)) {
      return JOY_POS_UP;
    }
    else if (isJoyStickDown(thresh)) {
      return JOY_POS_DOWN;      
    }
    else {
      return JOY_POS_NEUTRAL;
    }
  }

  int readJoyX() {
    return (int) joyX - zeroJoyX;
  }

  int readJoyY() {
    return (int)joyY - zeroJoyY;
  }


  // R, the radius, generally hovers around 210 (at least it does with mine)
  // int R() {
  //     return sqrt(readAccelX() * readAccelX() +readAccelY() * readAccelY() + readAccelZ() * readAccelZ());  
  // }


  // returns roll degrees
  int readRoll() {
    return (int)(atan2(readAccelX(),readAccelZ())/ M_PI * 180.0);
  }

  // returns pitch in degrees
  int readPitch() {        
    return (int) (acos(readAccelY()/RADIUS)/ M_PI * 180.0);  // optionally swap 'RADIUS' for 'R()'
  }

private:
  byte _nunchuk_decode_byte (byte x)
  {
    x = (x ^ 0x17) + 0x17;
    return x;
  }

  void _send_zero()
  {
    Wire.beginTransmission (0x52);	// transmit to device 0x52
#if (ARDUINO >= 100)
    Wire.write((uint8_t)0x00);// sends one byte
#else
    Wire.send((uint8_t)0x00);// sends one byte
#endif
    Wire.endTransmission ();	// stop transmitting
  }

};
#endif



