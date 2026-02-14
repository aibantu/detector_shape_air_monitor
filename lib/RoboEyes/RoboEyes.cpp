/*
  RoboEyes.cpp - Library for displaying animated robotic eyes on TFT displays using TFT_eSPI library.
  Created by Your Name, June 10, 2024.
  Released into the public domain.
*/
#include <RoboEyes.h>

// Display colors 适配TFT彩色
uint16_t BGCOLOR = 0; // background and overlays
uint16_t MAINCOLOR = 1; // drawings


//*********************************************************************************************
//  GENERAL METHODS
//*********************************************************************************************

// Constructor: takes a reference to the active Adafruit display object (e.g., Adafruit_SSD1327)
RoboEyes::RoboEyes(TFT_eSprite &disp, int x, int y, int w, int h) 
: sprite(&disp), eyeAreaX(x), eyeAreaY(y), eyeAreaW(w), eyeAreaH(h) {

    // tft->setRotation(rotation);
    // screenWidth = tft->width();
    // screenHeight = tft->height();

    // eyeAreaX = (screenWidth - eyeAreaW) / 2;
    // eyeAreaY = (screenHeight - eyeAreaH) /2;
    // EYE LEFT - Coordinates
    eyeLxDefault = (eyeAreaW - (eyeLwidthDefault + spaceBetweenDefault + eyeRwidthDefault))  /2;
    eyeLyDefault = ((eyeAreaH - eyeLheightDefault) / 2);
    eyeLx = eyeLxDefault;
    eyeLy = eyeLyDefault;
    eyeLxNext = eyeLx;
    eyeLyNext = eyeLy;

    // EYE RIGHT - Coordinates
    eyeRxDefault = eyeLxDefault + eyeLwidthDefault + spaceBetweenDefault;
    eyeRyDefault = eyeLyDefault;
    eyeRx = eyeRxDefault;
    eyeRy = eyeRyDefault;
    eyeRxNext = eyeRx;
    eyeRyNext = eyeRy;
}

//析构函数，释放内存
RoboEyes::~RoboEyes() {
    if (sprite != nullptr) {
        sprite->deleteSprite();
        sprite = nullptr;
    }
}

// Startup RoboEyes with defined screen-width, screen-height and max. frames per second
void RoboEyes::begin(byte frameRate) {
    sprite->setColorDepth(16); // use 16-bit color depth
    sprite->createSprite(eyeAreaW, eyeAreaH);
    sprite->fillSprite(BGCOLOR); // fill the sprite with background color
    eyeLheightCurrent = 1; // start with closed eyes
    eyeRheightCurrent = 1; // start with closed eyes
    setFramerate(frameRate); // calculate frame interval based on defined frameRate
}

void RoboEyes::update() {
    // Limit drawing updates to defined max framerate
    if(millis()-fpsTimer >= frameInterval){
        drawEyes();
        sprite->pushSprite(eyeAreaX, eyeAreaY); // push the complete frame in one go
        fpsTimer = millis();
    }
}

//*********************************************************************************************
//  SETTERS METHODS
//*********************************************************************************************

// Calculate frame interval based on defined frameRate
void RoboEyes::setFramerate(byte fps) {
    frameInterval = 1000/fps;
}

// Set color values
void RoboEyes::setDisplayColors(uint16_t background, uint16_t main) {
    BGCOLOR = background; // background and overlays, choose 0 for monochrome displays and 0x00 for grayscale displays such as SSD1322
    MAINCOLOR = main; // drawings, choose 1 for monochrome displays and 0x0F for grayscale displays such as SSD1322 (0x0F = maximum brightness)
}

void RoboEyes::setWidth(byte leftEye, byte rightEye) {
    int maxEyeW = eyeAreaW / 2 - spaceBetweenDefault;
    eyeLwidthNext = constrain(leftEye, 10, maxEyeW);
    eyeRwidthNext = constrain(rightEye, 10, maxEyeW);
    eyeLwidthDefault = eyeLwidthNext;
    eyeRwidthDefault = eyeRwidthNext;
}

void RoboEyes::setHeight(byte leftEye, byte rightEye) {
    int maxEyeH = eyeAreaH;
    eyeLheightNext = constrain(leftEye, 10, maxEyeH);
    eyeRheightNext = constrain(rightEye, 10, maxEyeH);
    eyeLheightDefault = eyeLheightNext;
    eyeRheightDefault = eyeRheightNext;
}

// Set border radius for left and right eye
void RoboEyes::setBorderradius(byte leftEye, byte rightEye) {
    eyeLborderRadiusNext = leftEye;
    eyeRborderRadiusNext = rightEye;
    eyeLborderRadiusDefault = leftEye;
    eyeRborderRadiusDefault = rightEye;
}

// Set space between the eyes, can also be negative
void RoboEyes::setSpacebetween(int space) {
    spaceBetweenNext = space;
    spaceBetweenDefault = space;
}

// Set mood expression
void RoboEyes::setMood(unsigned char mood) {
    switch (mood) {
        case ROBO_TIRED: tired=1; angry=0; happy=0; break;
        case ROBO_ANGRY: tired=0; angry=1; happy=0; break;
        case ROBO_HAPPY: tired=0; angry=0; happy=1; break;
        default: tired=0; angry=0; happy=0; break;
    }
}

// Set predefined position
void RoboEyes::setPosition(unsigned char position) {

    int maxXW = getScreenConstraint_X();
    int maxYH = getScreenConstraint_Y();

    switch (position) {
        case EYES_N:
        // North, top center
        eyeLxNext = maxXW / 2;
        eyeLyNext = 0;
        break;
        case EYES_NE:
        // North-east, top right
        eyeLxNext = maxXW;
        eyeLyNext = 0;
        break;
        case EYES_E:
        // East, middle right
        eyeLxNext = maxXW;
        eyeLyNext = maxYH / 2;
        break;
        case EYES_SE:
        // South-east, bottom right
        eyeLxNext = maxXW;
        eyeLyNext = maxYH;
        break;
        case EYES_S:
        // South, bottom center
        eyeLxNext = maxXW / 2;
        eyeLyNext = maxYH;
        break;
        case EYES_SW:
        // South-west, bottom left
        eyeLxNext = 0;
        eyeLyNext = maxYH;
        break;
        case EYES_W:
        // West, middle left
        eyeLxNext = 0;
        eyeLyNext = maxYH / 2;
        break;
        case EYES_NW:
        // North-west, top left
        eyeLxNext = 0;
        eyeLyNext = 0;
        break;
        default:
        // Middle center
        eyeLxNext = eyeLxDefault;
        eyeLyNext = eyeLyDefault;
        break;
    }
}

// Set automated eye blinking, minimal blink interval in full seconds and blink interval variation range in full seconds
void RoboEyes::setAutoblinker(bool active, int interval, int variation) {
    autoblinker = active;
    blinkInterval = interval;
    blinkIntervalVariation = variation;
}
void RoboEyes::setAutoblinker(bool active) {
    autoblinker = active;
}

// Set idle mode - automated eye repositioning, minimal time interval in full seconds and time interval variation range in full seconds
void RoboEyes::setIdleMode(bool active, int interval, int variation) {
    idle = active;
    idleInterval = interval;
    idleIntervalVariation = variation;
}
void RoboEyes::setIdleMode(bool active) {
    idle = active;
}

// Set curious mode - the respectively outer eye gets larger when looking left or right
void RoboEyes::setCuriosity(bool curiousBit) {
    curious = curiousBit;
}

// Set cyclops mode - show only one eye 
void RoboEyes::setCyclops(bool cyclopsBit) {
    cyclops = cyclopsBit;
}

// Set horizontal flickering (displacing eyes left/right)
void RoboEyes::setHFlicker (bool flickerBit, byte Amplitude) {
    hFlicker = flickerBit; // turn flicker on or off
    hFlickerAmplitude = Amplitude; // define amplitude of flickering in pixels
}

void RoboEyes::setHFlicker (bool flickerBit) {
    hFlicker = flickerBit; // turn flicker on or off
}

// Set vertical flickering (displacing eyes up/down)
void RoboEyes::setVFlicker (bool flickerBit, byte Amplitude) {
    vFlicker = flickerBit; // turn flicker on or off
    vFlickerAmplitude = Amplitude; // define amplitude of flickering in pixels
}

void RoboEyes::setVFlicker (bool flickerBit) {
    vFlicker = flickerBit; // turn flicker on or off
}

void RoboEyes::setSweat (bool sweatBit) {
    sweat = sweatBit; // turn sweat on or off
}

void RoboEyes::setEyeArea(int xStart, int yStart, int areaW, int areaH) {
    eyeAreaX = xStart;
    eyeAreaY = yStart;
    eyeAreaW = areaW;
    eyeAreaH = areaH;

    eyeLxDefault = ((eyeAreaW)-(eyeLwidthDefault+spaceBetweenDefault+eyeRwidthDefault))/2;
    eyeLyDefault = ((eyeAreaH-eyeLheightDefault)/2);
    eyeRxDefault = eyeLxDefault + eyeLwidthDefault + spaceBetweenDefault;
    eyeRyDefault = eyeLyDefault;

    if (sprite != nullptr) {
        sprite->deleteSprite();
        sprite->createSprite(eyeAreaW, eyeAreaH);
    }
}

//*********************************************************************************************
//  GETTERS METHODS
//*********************************************************************************************

// Returns the max x position for left eye
int RoboEyes::getScreenConstraint_X() {
    // return screenWidth-eyeLwidthCurrent-spaceBetweenCurrent-eyeRwidthCurrent;
    if (cyclops) {
        return eyeAreaW - eyeLwidthCurrent;
    } else {
        return (eyeAreaW - eyeLwidthCurrent - spaceBetweenCurrent - eyeRwidthCurrent);
    }
} 

// Returns the max y position for left eye
int RoboEyes::getScreenConstraint_Y() {
    //  return screenHeight-eyeLheightDefault; // using default height here, because height will vary when blinking and in curious mode
    return eyeAreaH - eyeLheightDefault; 
}

// 获取流汗状态（新增）
bool RoboEyes::getSweat() {
    return sweat; // 返回私有变量sweat的值
}

//*********************************************************************************************
//  BASIC ANIMATION METHODS
//*********************************************************************************************

// BLINKING FOR BOTH EYES AT ONCE
// Close both eyes
void RoboEyes::close() {
    eyeLheightNext = 1; // closing left eye
    eyeRheightNext = 1; // closing right eye
    eyeL_open = 0; // left eye not opened (=closed)
    eyeR_open = 0; // right eye not opened (=closed)
}

// Open both eyes
void RoboEyes::open() {
    eyeL_open = 1; // left eye opened - if true, drawEyes() will take care of opening eyes again
    eyeR_open = 1; // right eye opened
    // eyeLheightNext = eyeLheightDefault; //认为需要删除
    // eyeRheightNext = eyeRheightDefault;
}

// Trigger eyeblink animation
void RoboEyes::blink() {
    close();
    open();
}

// BLINKING FOR SINGLE EYES, CONTROL EACH EYE SEPARATELY
// Close eye(s)
void RoboEyes::close(bool left, bool right) {
    if(left){
        eyeLheightNext = 1; // blinking left eye
        eyeL_open = 0; // left eye not opened (=closed)
    }
    if(right){
        eyeRheightNext = 1; // blinking right eye
        eyeR_open = 0; // right eye not opened (=closed)
    }
}

// Open eye(s)
void RoboEyes::open(bool left, bool right) {
    if(left){
        eyeL_open = 1; // left eye opened - if true, drawEyes() will take care of opening eyes again
    }
    if(right){
        eyeR_open = 1; // right eye opened
    }
}

// Trigger eyeblink(s) animation
void RoboEyes::blink(bool left, bool right) {
    close(left, right);
    open(left, right);
}

//*********************************************************************************************
//  MACRO ANIMATION METHODS
//*********************************************************************************************

// Play confused animation - one shot animation of eyes shaking left and right
void RoboEyes::anim_confused() {
    confused = 1;
}

// Play laugh animation - one shot animation of eyes shaking up and down
void RoboEyes::anim_laugh() {
    laugh = 1;
}

//*********************************************************************************************
//  PRE-CALCULATIONS AND ACTUAL DRAWINGS
//*********************************************************************************************

void RoboEyes::drawEyes(){

    //// PRE-CALCULATIONS - EYE SIZES AND VALUES FOR ANIMATION TWEENINGS ////

    // Vertical size offset for larger eyes when looking left or right (curious gaze)
    if(curious){
        if(eyeLxNext <= 10){eyeLheightOffset=8;}
        else if (eyeLxNext >= (eyeAreaW - eyeLwidthCurrent -10) && cyclops){eyeLheightOffset=8;}
        else{eyeLheightOffset=0;} // left eye
        if(eyeRxNext >= eyeAreaW -eyeRwidthCurrent-10){eyeRheightOffset=8;}
        else{eyeRheightOffset=0;} // right eye
    } else {
        eyeLheightOffset=0; // reset height offset for left eye
        eyeRheightOffset=0; // reset height offset for right eye
    }

    // Left eye height
    eyeLheightCurrent = (eyeLheightCurrent + eyeLheightNext + eyeLheightOffset)/2;
    eyeLy+= ((eyeLheightDefault-eyeLheightCurrent)/2); // vertical centering of eye when closing
    eyeLy-= eyeLheightOffset/2;
    // Right eye height
    eyeRheightCurrent = (eyeRheightCurrent + eyeRheightNext + eyeRheightOffset)/2;
    eyeRy+= (eyeRheightDefault-eyeRheightCurrent)/2; // vertical centering of eye when closing
    eyeRy-= eyeRheightOffset/2;

    // Open eyes again after closing them
    if(eyeL_open){
        if(eyeLheightCurrent <= 1 + eyeLheightOffset){eyeLheightNext = eyeLheightDefault;} 
    }
    if(eyeR_open){
        if(eyeRheightCurrent <= 1 + eyeRheightOffset){eyeRheightNext = eyeRheightDefault;} 
    }

    // Left eye width
    eyeLwidthCurrent = (eyeLwidthCurrent + eyeLwidthNext)/2;
    // Right eye width
    eyeRwidthCurrent = (eyeRwidthCurrent + eyeRwidthNext)/2;

    // Space between eyes
    spaceBetweenCurrent = (spaceBetweenCurrent + spaceBetweenNext)/2;

    // Left eye coordinates
    eyeLx = (eyeLx + eyeLxNext)/2;
    eyeLy = (eyeLy + eyeLyNext)/2;
    // Right eye coordinates
    eyeRxNext = eyeLxNext+eyeLwidthCurrent+spaceBetweenCurrent; // right eye's x position depends on left eyes position + the space between
    eyeRyNext = eyeLyNext; // right eye's y position should be the same as for the left eye
    eyeRx = (eyeRx + eyeRxNext)/2;
    eyeRy = (eyeRy + eyeRyNext)/2;

    // 计算左眼在精灵内的最大X/Y（精灵宽度-左眼宽度，确保完全容纳）
    int maxEyeLX = eyeAreaW - eyeLwidthCurrent;
    int maxEyeLY = eyeAreaH - eyeLheightDefault; // 用默认高度，避免眨眼时超出
    // 约束左眼坐标：X∈[0, maxEyeLX]，Y∈[0, maxEyeLY]（精灵内相对坐标范围）
    eyeLx = constrain(eyeLx, 0, maxEyeLX);
    eyeLy = constrain(eyeLy, 0, maxEyeLY);

    // 非独眼模式：约束右眼坐标
    if (!cyclops) {
        int maxEyeRX = eyeAreaW - eyeRwidthCurrent;
        int maxEyeRY = eyeAreaH - eyeRheightDefault;
        // 约束右眼坐标：X∈[0, maxEyeRX]，Y∈[0, maxEyeRY]
        eyeRx = constrain(eyeRx, 0, maxEyeRX);
        eyeRy = constrain(eyeRy, 0, maxEyeRY);
    }

    // Left eye border radius
    eyeLborderRadiusCurrent = (eyeLborderRadiusCurrent + eyeLborderRadiusNext)/2;
    // Right eye border radius
    eyeRborderRadiusCurrent = (eyeRborderRadiusCurrent + eyeRborderRadiusNext)/2;

    //// APPLYING MACRO ANIMATIONS ////

    if(autoblinker){
        if(millis() >= blinktimer){
        blink();
        blinktimer = millis()+(blinkInterval*1000)+(random(blinkIntervalVariation)*1000); // calculate next time for blinking
        }
    }

    // Laughing - eyes shaking up and down for the duration defined by laughAnimationDuration (default = 500ms)
    if(laugh){
        if(laughToggle){
        setVFlicker(1, 5);
        laughAnimationTimer = millis();
        laughToggle = 0;
        } else if(millis() >= laughAnimationTimer+laughAnimationDuration){
        setVFlicker(0, 0);
        laughToggle = 1;
        laugh=0; 
        }
    }

    // Confused - eyes shaking left and right for the duration defined by confusedAnimationDuration (default = 500ms)
    if(confused){
        if(confusedToggle){
            setHFlicker(1, 20);
            confusedAnimationTimer = millis();
            confusedToggle = 0;
        } else if(millis() >= confusedAnimationTimer+confusedAnimationDuration){
            setHFlicker(0, 0);
            confusedToggle = 1;
            confused=0; 
        }
    }

    // Idle - eyes moving to random positions on screen
    if(idle){
        if(millis() >= idleAnimationTimer){
            eyeLxNext = random(0, getScreenConstraint_X());
            eyeLyNext = random(0, getScreenConstraint_Y());
            idleAnimationTimer = millis()+(idleInterval*1000)+(random(idleIntervalVariation)*1000); // calculate next time for eyes repositioning
        }
    }

    // Adding offsets for horizontal flickering/shivering
    if(hFlicker){
        if(hFlickerAlternate) {
            eyeLx += hFlickerAmplitude;
            eyeRx += hFlickerAmplitude;
        } else {
            eyeLx -= hFlickerAmplitude;
            eyeRx -= hFlickerAmplitude;
        }
        hFlickerAlternate = !hFlickerAlternate;
    }

    // Adding offsets for horizontal flickering/shivering
    if(vFlicker){
        if(vFlickerAlternate) {
            eyeLy += vFlickerAmplitude;
            eyeRy += vFlickerAmplitude;
        } else {
            eyeLy -= vFlickerAmplitude;
            eyeRy -= vFlickerAmplitude;
        }
        vFlickerAlternate = !vFlickerAlternate;
    }

    // Cyclops mode, set second eye's size and space between to 0
    if(cyclops){
        eyeRwidthCurrent = 0;
        eyeRheightCurrent = 0;
        spaceBetweenCurrent = 0;
    }

    //// ACTUAL DRAWINGS ////
    sprite->fillSprite(BGCOLOR); // start with a blank screen

    // Draw basic eye rectangles
    sprite->fillRoundRect(eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent, eyeLborderRadiusCurrent, MAINCOLOR); // left eye
    if (!cyclops){
        sprite->fillRoundRect(eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent, eyeRborderRadiusCurrent, MAINCOLOR); // right eye
    }

    // Prepare mood type transitions
    if (tired){eyelidsTiredHeightNext = eyeLheightCurrent/2; eyelidsAngryHeightNext = 0;} else{eyelidsTiredHeightNext = 0;}
    if (angry){eyelidsAngryHeightNext = eyeLheightCurrent/2; eyelidsTiredHeightNext = 0;} else{eyelidsAngryHeightNext = 0;}
    if (happy){eyelidsHappyBottomOffsetNext = eyeLheightCurrent/2;} else{eyelidsHappyBottomOffsetNext = 0;}

    // Draw tired top eyelids 
    eyelidsTiredHeight = (eyelidsTiredHeight + eyelidsTiredHeightNext)/2;

    if (!cyclops){
        sprite->fillTriangle(eyeLx, eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy -1, eyeLx, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // left eye 
        sprite->fillTriangle(eyeRx, eyeRy -1, eyeRx+eyeRwidthCurrent, eyeRy -1, eyeRx+eyeRwidthCurrent, eyeRy+eyelidsTiredHeight-1, BGCOLOR); // right eye
    } else {
        // Cyclops tired eyelids
        sprite->fillTriangle(eyeLx, eyeLy -1, eyeLx+(eyeLwidthCurrent/2), eyeLy -1, eyeLx, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // left eyelid half
        sprite->fillTriangle(eyeLx+(eyeLwidthCurrent/2), eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy+eyelidsTiredHeight-1, BGCOLOR); // right eyelid half
    }

    // Draw angry top eyelids 
    eyelidsAngryHeight = (eyelidsAngryHeight + eyelidsAngryHeightNext)/2;
    if (!cyclops){ 
        sprite->fillTriangle(eyeLx, eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy+eyelidsAngryHeight-1, BGCOLOR); // left eye
        sprite->fillTriangle(eyeRx, eyeRy -1, eyeRx+eyeRwidthCurrent, eyeRy -1, eyeRx, eyeRy+eyelidsAngryHeight-1, BGCOLOR); // right eye
    } else {
        // Cyclops angry eyelids
        sprite->fillTriangle(eyeLx, eyeLy -1, eyeLx+(eyeLwidthCurrent/2), eyeLy -1, eyeLx+(eyeLwidthCurrent/2), eyeLy+eyelidsAngryHeight-1, BGCOLOR); // left eyelid half
        sprite->fillTriangle(eyeLx+(eyeLwidthCurrent/2), eyeLy -1, eyeLx+eyeLwidthCurrent, eyeLy -1, eyeLx+(eyeLwidthCurrent/2), eyeLy+eyelidsAngryHeight-1, BGCOLOR); // right eyelid half
    }

    // Draw happy bottom eyelids
    eyelidsHappyBottomOffset = (eyelidsHappyBottomOffset + eyelidsHappyBottomOffsetNext)/2;
    sprite->fillRoundRect(eyeLx-1, (eyeLy+eyeLheightCurrent)-eyelidsHappyBottomOffset+1, eyeLwidthCurrent+2, eyeLheightDefault, eyeLborderRadiusCurrent, BGCOLOR); // left eye
    if (!cyclops){ 
        sprite->fillRoundRect(eyeRx-1, (eyeRy+eyeRheightCurrent)-eyelidsHappyBottomOffset+1, eyeRwidthCurrent+2, eyeRheightDefault, eyeRborderRadiusCurrent, BGCOLOR); // right eye
    }

    // Add sweat drops
    if (sweat){
        // Sweat drop 1 -> left corner
        if(sweat1YPos <= sweat1YPosMax){sweat1YPos+=0.5;} // vertical movement from initial to max
        else {sweat1XPosInitial = random(30); sweat1YPos = 2; sweat1YPosMax = (random(10)+10); sweat1Width = 1; sweat1Height = 2;} // if max vertical position is reached: reset all values for next drop
        if(sweat1YPos <= sweat1YPosMax/2){sweat1Width+=0.5; sweat1Height+=0.5;} // shape grows in first half of animation ...
        else {sweat1Width-=0.1; sweat1Height-=0.5;} // ... and shrinks in second half of animation
        sweat1XPos = sweat1XPosInitial-(sweat1Width/2); // keep the growing shape centered to initial x position
        sprite->fillRoundRect(sweat1XPos, sweat1YPos, sweat1Width, sweat1Height, sweatBorderradius, MAINCOLOR); // draw sweat drop


        // Sweat drop 2 -> center area
        if(sweat2YPos <= sweat2YPosMax){sweat2YPos+=0.5;} // vertical movement from initial to max
        else {sweat2XPosInitial = random((eyeAreaW-60))+30; sweat2YPos = 2; sweat2YPosMax = (random(10)+10); sweat2Width = 1; sweat2Height = 2;} // if max vertical position is reached: reset all values for next drop
        if(sweat2YPos <= sweat2YPosMax/2){sweat2Width+=0.5; sweat2Height+=0.5;} // shape grows in first half of animation ...
        else {sweat2Width-=0.1; sweat2Height-=0.5;} // ... and shrinks in second half of animation
        sweat2XPos = sweat2XPosInitial-(sweat2Width/2); // keep the growing shape centered to initial x position
        sprite->fillRoundRect(sweat2XPos, sweat2YPos, sweat2Width, sweat2Height, sweatBorderradius, MAINCOLOR); // draw sweat drop


        // Sweat drop 3 -> right corner
        if(sweat3YPos <= sweat3YPosMax){sweat3YPos+=0.5;} // vertical movement from initial to max
        else {sweat3XPosInitial = (eyeAreaW-30)+(random(30)); sweat3YPos = 2; sweat3YPosMax = (random(10)+10); sweat3Width = 1; sweat3Height = 2;} // if max vertical position is reached: reset all values for next drop
        if(sweat3YPos <= sweat3YPosMax/2){sweat3Width+=0.5; sweat3Height+=0.5;} // shape grows in first half of animation ...
        else {sweat3Width-=0.1; sweat3Height-=0.5;} // ... and shrinks in second half of animation
        sweat3XPos = sweat3XPosInitial-(sweat3Width/2); // keep the growing shape centered to initial x position
        sprite->fillRoundRect(sweat3XPos, sweat3YPos, sweat3Width, sweat3Height, sweatBorderradius, MAINCOLOR); // draw sweat drop
    }

    // display->display(); // show drawings on display

} // end of drawEyes method


