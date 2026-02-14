/*
  RoboEyes.h - Library for displaying animated robotic eyes on TFT displays using TFT_eSPI library.
  Created by Your Name, June 10, 2024.
  Released into the public domain.
*/

#ifndef _ROBOEYES_H
#define _ROBOEYES_H

#include <TFT_eSPI.h>

// Display colors 适配TFT彩色
extern uint16_t BGCOLOR; // background and overlays
extern uint16_t MAINCOLOR; // drawings

// For mood type switch
#define ROBO_DEFAULT 0
#define ROBO_TIRED 1
#define ROBO_ANGRY 2
#define ROBO_HAPPY 3

// For turning things on or off
#define ON 1
#define OFF 0

// For switch "predefined positions"
#define EYES_N 1 // north, top center
#define EYES_NE 2 // north-east, top right
#define EYES_E 3 // east, middle right
#define EYES_SE 4 // south-east, bottom right
#define EYES_S 5 // south, bottom center
#define EYES_SW 6 // south-west, bottom left
#define EYES_W 7 // west, middle left
#define EYES_NW 8 // north-west, top left 
// for middle center set "ROBO_DEFAULT"


// Constructor: takes a reference to the active Adafruit display object (e.g., Adafruit_SSD1327)

class RoboEyes
{
  private:

  // Reference to Adafruit display object

  // TFT_eSPI *tft;
  TFT_eSprite *sprite;

  // For general setup - screen size and max. frame rate
  // int screenWidth;
  // int screenHeight;
  int frameInterval = 50; // default value for 50 frames per second (1000/50 = 20 milliseconds)
  unsigned long fpsTimer = 0; // for timing the frames per second

  // 配置眼睛显示区域（动态配置：从构造函数传入）
  int eyeAreaX;    // 显示区域起始X（如80）
  int eyeAreaY;    // 显示区域起始Y（如6）
  int eyeAreaW;    // 显示区域宽度（如160）
  int eyeAreaH;    // 显示区域高度（如160）

  // For controlling mood types and expressions
  bool tired = 0;
  bool angry = 0;
  bool happy = 0;
  bool curious = 0; // if true, draw the outer eye larger when looking left or right
  bool cyclops = 0; // if true, draw only one eye
  bool eyeL_open = 0; // left eye opened or closed?
  bool eyeR_open = 0; // right eye opened or closed?


  //*********************************************************************************************
  //  Eyes Geometry
  //*********************************************************************************************

  // EYE LEFT - size and border radius
  int eyeLwidthDefault = 50;
  int eyeLheightDefault = 50;
  int eyeLwidthCurrent = eyeLwidthDefault;
  int eyeLheightCurrent = 1; // start with closed eye, otherwise set to eyeLheightDefault
  int eyeLwidthNext = eyeLwidthDefault;
  int eyeLheightNext = eyeLheightDefault;
  int eyeLheightOffset = 0;
  // Border Radius
  byte eyeLborderRadiusDefault = 12;
  byte eyeLborderRadiusCurrent = eyeLborderRadiusDefault;
  byte eyeLborderRadiusNext = eyeLborderRadiusDefault;

  // EYE RIGHT - size and border radius
  int eyeRwidthDefault = eyeLwidthDefault;
  int eyeRheightDefault = eyeLheightDefault;
  int eyeRwidthCurrent = eyeRwidthDefault;
  int eyeRheightCurrent = 1; // start with closed eye, otherwise set to eyeRheightDefault
  int eyeRwidthNext = eyeRwidthDefault;
  int eyeRheightNext = eyeRheightDefault;
  int eyeRheightOffset = 0;
  // Border Radius
  byte eyeRborderRadiusDefault = 12;
  byte eyeRborderRadiusCurrent = eyeRborderRadiusDefault;
  byte eyeRborderRadiusNext = eyeRborderRadiusDefault;

  // Space between eyes
  int spaceBetweenDefault = 20;
  int spaceBetweenCurrent = spaceBetweenDefault;
  int spaceBetweenNext = spaceBetweenDefault;

  // EYE LEFT - Coordinates
  int eyeLxDefault;
  int eyeLyDefault;
  int eyeLx = eyeLxDefault;
  int eyeLy = eyeLyDefault;
  int eyeLxNext = eyeLx;
  int eyeLyNext = eyeLy;

  // EYE RIGHT - Coordinates
  int eyeRxDefault;
  int eyeRyDefault;
  int eyeRx = eyeRxDefault;
  int eyeRy = eyeRyDefault;
  int eyeRxNext = eyeRx;
  int eyeRyNext = eyeRy;

  // BOTH EYES 
  // Eyelid top size
  byte eyelidsHeightMax = eyeLheightDefault/2; // top eyelids max height
  byte eyelidsTiredHeight = 0;
  byte eyelidsTiredHeightNext = eyelidsTiredHeight;
  byte eyelidsAngryHeight = 0;
  byte eyelidsAngryHeightNext = eyelidsAngryHeight;
  // Bottom happy eyelids offset
  byte eyelidsHappyBottomOffsetMax = (eyeLheightDefault/2)+3;
  byte eyelidsHappyBottomOffset = 0;
  byte eyelidsHappyBottomOffsetNext = 0;



  //*********************************************************************************************
  //  Macro Animations
  //*********************************************************************************************

  // Animation - horizontal flicker/shiver
  bool hFlicker = 0;
  bool hFlickerAlternate = 0;
  byte hFlickerAmplitude = 2;

  // Animation - vertical flicker/shiver
  bool vFlicker = 0;
  bool vFlickerAlternate = 0;
  byte vFlickerAmplitude = 10;

  // Animation - auto blinking
  bool autoblinker = 0; // activate auto blink animation
  int blinkInterval = 1; // basic interval between each blink in full seconds
  int blinkIntervalVariation = 4; // interval variaton range in full seconds, random number inside of given range will be add to the basic blinkInterval, set to 0 for no variation
  unsigned long blinktimer = 0; // for organising eyeblink timing

  // Animation - idle mode: eyes looking in random directions
  bool idle = 0;
  int idleInterval = 1; // basic interval between each eye repositioning in full seconds
  int idleIntervalVariation = 3; // interval variaton range in full seconds, random number inside of given range will be add to the basic idleInterval, set to 0 for no variation
  unsigned long idleAnimationTimer = 0; // for organising eyeblink timing

  // Animation - eyes confused: eyes shaking left and right
  bool confused = 0;
  unsigned long confusedAnimationTimer = 0;
  int confusedAnimationDuration = 500;
  bool confusedToggle = 1;

  // Animation - eyes laughing: eyes shaking up and down
  bool laugh = 0;
  unsigned long laughAnimationTimer = 0;
  int laughAnimationDuration = 500;
  bool laughToggle = 1;

  // Animation - sweat on the forehead
  bool sweat = 0;
  byte sweatBorderradius = 3;

  // Sweat drop 1
  int sweat1XPosInitial = 2;
  int sweat1XPos;
  float sweat1YPos = 2;
  int sweat1YPosMax;
  float sweat1Height = 2;
  float sweat1Width = 1;

  // Sweat drop 2
  int sweat2XPosInitial = 2;
  int sweat2XPos;
  float sweat2YPos = 2;
  int sweat2YPosMax;
  float sweat2Height = 2;
  float sweat2Width = 1;

  // Sweat drop 3
  int sweat3XPosInitial = 2;
  int sweat3XPos;
  float sweat3YPos = 2;
  int sweat3YPosMax;
  float sweat3Height = 2;
  float sweat3Width = 1;

  //*********************************************************************************************
  //  PRE-CALCULATIONS AND ACTUAL DRAWINGS
  //*********************************************************************************************

  void drawEyes();

  public:
  //*********************************************************************************************
  //  GENERAL METHODS
  //*********************************************************************************************

  RoboEyes(TFT_eSprite &disp, int x, int y, int w, int h);
  ~RoboEyes();

  // Startup RoboEyes with defined screen-width, screen-height and max. frames per second
  void begin(byte frameRate);

  void update();

  //*********************************************************************************************
  //  SETTERS METHODS
  //*********************************************************************************************

  // Calculate frame interval based on defined frameRate
  void setFramerate(byte fps);

  // Set color values
  void setDisplayColors(uint16_t background, uint16_t main);

  void setWidth(byte leftEye, byte rightEye);

  void setHeight(byte leftEye, byte rightEye) ;

  // Set border radius for left and right eye
  void setBorderradius(byte leftEye, byte rightEye);

  // Set space between the eyes, can also be negative
  void setSpacebetween(int space);

  // Set mood expression
  void setMood(unsigned char mood);

  // Set predefined position
  void setPosition(unsigned char position);

  // Set automated eye blinking, minimal blink interval in full seconds and blink interval variation range in full seconds
  void setAutoblinker(bool active, int interval, int variation);
  void setAutoblinker(bool active);

  // Set idle mode - automated eye repositioning, minimal time interval in full seconds and time interval variation range in full seconds
  void setIdleMode(bool active, int interval, int variation);
  void setIdleMode(bool active);

  // Set curious mode - the respectively outer eye gets larger when looking left or right
  void setCuriosity(bool curiousBit);

  // Set cyclops mode - show only one eye 
  void setCyclops(bool cyclopsBit);

  // Set horizontal flickering (displacing eyes left/right)
  void setHFlicker (bool flickerBit, byte Amplitude);
  void setHFlicker (bool flickerBit);

  // Set vertical flickering (displacing eyes up/down)
  void setVFlicker (bool flickerBit, byte Amplitude);
  void setVFlicker (bool flickerBit);

  void setSweat (bool sweatBit);

  void setEyeArea(int xStart, int yStart, int areaW, int areaH);

  //*********************************************************************************************
  //  GETTERS METHODS
  //*********************************************************************************************

  // Returns the max x position for left eye
  int getScreenConstraint_X();

  // Returns the max y position for left eye
  int getScreenConstraint_Y();

  bool getSweat();

  //*********************************************************************************************
  //  BASIC ANIMATION METHODS
  //*********************************************************************************************

  // BLINKING FOR BOTH EYES AT ONCE
  // Close both eyes
  void close();

  // Open both eyes
  void open();

  // Trigger eyeblink animation
  void blink();

  // BLINKING FOR SINGLE EYES, CONTROL EACH EYE SEPARATELY
  // Close eye(s)
  void close(bool left, bool right);

  // Open eye(s)
  void open(bool left, bool right);

  // Trigger eyeblink(s) animation
  void blink(bool left, bool right);

  //*********************************************************************************************
  //  MACRO ANIMATION METHODS
  //*********************************************************************************************

  // Play confused animation - one shot animation of eyes shaking left and right
  void anim_confused();

  // Play laugh animation - one shot animation of eyes shaking up and down
  void anim_laugh();

}; // end of class roboEyes

#endif
