// Elegoo_TFTLCD - Version: Latest 
#include <Elegoo_TFTLCD.h>
#include <pin_magic.h>
#include <registers.h>

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#include <Elegoo_GFX.h>   
#include <TouchScreen.h>  
#include <EEPROM.h>

#include <elapsedMillis.h>
#include <SdFat.h>
#include <SPI.h>

//
// Pin numbers in templates must be constants.
const uint8_t SOFT_MISO_PIN = 12;
const uint8_t SOFT_MOSI_PIN = 11;
const uint8_t SOFT_SCK_PIN  = 13;
//
// Chip select may be constant or RAM variable.
const uint8_t SD_CHIP_SELECT_PIN = 10;

// SdFat software SPI template
SdFatSoftSpi<SOFT_MISO_PIN, SOFT_MOSI_PIN, SOFT_SCK_PIN> sd;

#define LCD_CS A3 
#define LCD_CD A2 
#define LCD_WR A1 
#define LCD_RD A0 
#define PIN_SD_CS 10 // Elegoo SD shields and modules: pin 10

#define LCD_RESET A4 

#define  BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define SENSIBILITY 300
#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define YP A3
#define XM A2 
#define YM 9 
#define XP 8 

#define TS_MINX 110 //600
#define TS_MINY 910 //900
#define TS_MAXX 930 //965
#define TS_MAXY 70  //140

#define MAX_BMP         10                      // bmp file num
#define FILENAME_LEN    20                      // max file name length

const int __Gnbmp_height = 320;                 // bmp hight
const int __Gnbmp_width  = 240;                 // bmp width

unsigned char __Gnbmp_image_offset  = 0;        // offset

int __Gnfile_num = 1;                           // num of file

char __Gsbmp_files[1][FILENAME_LEN] =           // add file name here
{
"mars.bmp"
};
File bmpFile;

#define PIN 31

#define VOLUME_DN_BTN 35
#define VOLUME_UP_BTN 41
#define PLAYPAUSE_BTN 37
#define NEXTTRACK_BTN 39
#define PREVTRACK_BTN 33

#define BTN_WIDTH 55
#define BTN_HEIGHT 50
#define BTN_ROW1 150
#define BTN_ROW2 250

// duration to hold button down, so we don't have to add delay in loop
#define ZERO_CHECK 50 // number of consecutive 0 readings after a button press before we believe user has let up.
#define BUTTON_PRESSED_DURATION 250

int numZerosAfterPress = 0;
int buttonPressedPin = 0;
elapsedMillis buttonPressedDuration;

#define NUM_LEDS 10

#define BRIGHTNESS 50

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRBW + NEO_KHZ800);

TouchScreen ts = TouchScreen(XP, YP, XM, YM, SENSIBILITY);


byte neopix_gamma[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };


/*********************************************/
// This procedure reads a bitmap and draws it to the screen
// its sped up by reading many pixels worth of data at a time
// instead of just one pixel at a time. increading the buffer takes
// more RAM but makes the drawing a little faster. 20 pixels' worth
// is probably a good place

#define BUFFPIXEL       60                      // must be a divisor of 240 
#define BUFFPIXEL_X3    180                     // BUFFPIXELx3

void bmpdraw(File* f, int x, int y)
{
    bmpFile.seek(__Gnbmp_image_offset);

    uint32_t time = millis();

    uint8_t sdbuffer[BUFFPIXEL_X3];                 // 3 * pixels to buffer

    for (int i=0; i< __Gnbmp_height; i++) {
        for(int j=0; j<(240/BUFFPIXEL); j++) {
            bmpFile.read(sdbuffer, BUFFPIXEL_X3);
            
            uint8_t buffidx = 0;
            int offset_x = j*BUFFPIXEL;
            unsigned int __color[BUFFPIXEL];
            
            for(int k=0; k<BUFFPIXEL; k++) {
                __color[k] = sdbuffer[buffidx+2]>>3;                        // read
                __color[k] = __color[k]<<6 | (sdbuffer[buffidx+1]>>2);      // green
                __color[k] = __color[k]<<5 | (sdbuffer[buffidx+0]>>3);      // blue
                
                buffidx += 3;
            }

      for (int m = 0; m < BUFFPIXEL; m ++) {
              tft.drawPixel(m+offset_x, i,__color[m]);
      }
        }
    }
    
    Serial.print(millis() - time, DEC);
    Serial.println(" ms");
}

boolean bmpReadHeader(File* f) 
{
    // read header
    uint32_t tmp;
    uint8_t bmpDepth;
    
    if (read16(f) != 0x4D42) {
        // magic bytes missing
        return false;
    }

    // read file size
    Serial.print("Pos before: ");
    Serial.println(f->curPosition());
    tmp = read32(f);
    Serial.print("Pos after: ");
    Serial.println(f->curPosition());
    Serial.print("size 0x");
    Serial.println(tmp, HEX);

    // read and ignore creator bytes
    read32(f);

    __Gnbmp_image_offset = read32(f);
    Serial.print("offset ");
    Serial.println(__Gnbmp_image_offset, HEX);

    // read DIB header
    tmp = read32(f);
    Serial.print("header size ");
    Serial.println(tmp, HEX);
    
    int bmp_width = read32(f);
    int bmp_height = read32(f);
    
    if(bmp_width != __Gnbmp_width || bmp_height != __Gnbmp_height)  {    // if image is not 320x240, return false
      Serial.print("BMP wrong size, expected: ");
      Serial.print(__Gnbmp_width);
      Serial.print(" got: ");
      Serial.println(bmp_width);
      return false;
    }

    if (read16(f) != 1) {
      Serial.println("Read16() != 1");
      return false;
    }

    bmpDepth = read16(f);
    Serial.print("bitdepth ");
    Serial.println(bmpDepth, DEC);

    if (read32(f) != 0) {
        // compression not supported!
        return false;
    }

    Serial.print("compression ");
    Serial.println(tmp, DEC);

    return true;
}

/*********************************************/
// These read data from the SD card file and convert them to big endian
// (the data is stored in little endian format!)

// LITTLE ENDIAN!
uint16_t read16(File* f)
{
    uint16_t d;
    uint8_t b;
    b = f->read();
    d = f->read();
    d <<= 8;
    d |= b;
    return d;
}

// LITTLE ENDIAN!
uint32_t read32(File* f)
{
    uint32_t d;
    uint16_t b;

    b = read16(f);
    d = read16(f);
    d <<= 16;
    d |= b;
    return d;
}


void setup() {
  // This is for Trinket 5V 16MHz, you can remove these three lines if you are not using a Trinket
  #if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
  #endif
  
  Serial.begin(115200);

  
  // End of trinket special code
  strip.setBrightness(BRIGHTNESS);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  
  // touchscreen
  tft.reset();
  tft.begin(0x9341); 

  tft.setRotation(4); 
  
  tft.fillScreen(BLUE);
  
  if (!sd.begin(SD_CHIP_SELECT_PIN)) {
    sd.initErrorHalt();
    Serial.println("initialization failed!");
    tft.setCursor(0, 0);
    tft.setTextColor(WHITE);    
    tft.setTextSize(1);
    tft.println("SD Card Init fail.");   
  }else
    Serial.println("initialization done."); 
  
  pinMode(VOLUME_UP_BTN, OUTPUT);
  pinMode(VOLUME_DN_BTN,OUTPUT);
  pinMode(PLAYPAUSE_BTN, OUTPUT);
  pinMode(NEXTTRACK_BTN, OUTPUT);
  pinMode(PREVTRACK_BTN, OUTPUT);

  int i = 0;
  bmpFile = sd.open(__Gsbmp_files[i]);
  if (!bmpFile) {
      Serial.print("didn't find image: ");
      Serial.println(__Gsbmp_files[i]);
      tft.setTextColor(WHITE);    
      tft.setTextSize(1);
      tft.print("didn't find image: ");
      tft.println(__Gsbmp_files[i]);
      while (1);
  }

  if(!bmpReadHeader(&bmpFile)) {
      Serial.println("bad bmp");
      tft.setTextColor(WHITE);    
      tft.setTextSize(1);
      tft.print("bad bmp: ");
      tft.println(__Gsbmp_files[i]);
  } else {
    bmpdraw(&bmpFile, 0, 0);
  }

  bmpFile.close();

//  // Volume Down Button  
//  tft.fillRect(135, BTN_ROW2, BTN_WIDTH, BTN_HEIGHT, GREEN);
//  // Play/Pause Button
//  tft.fillRect(90, BTN_ROW1, BTN_WIDTH, BTN_HEIGHT, YELLOW);
//  // Volume Up Button
//  tft.fillRect(55, BTN_ROW2, BTN_WIDTH, BTN_HEIGHT, BLUE);
//  // Prev Track Button
//  tft.fillRect(15, BTN_ROW1, BTN_WIDTH, BTN_HEIGHT, CYAN);
//  // Next Trace
//  tft.fillRect(165, BTN_ROW1, BTN_WIDTH, BTN_HEIGHT, MAGENTA);
}

void loop() {
  processLights();
  processButtonDisplay();
}

void processButtonDisplay() {
  // touchscreen touch functionality
  digitalWrite(14, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(14, LOW);
  
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    numZerosAfterPress = 0;
    // Width = 240 height = 340
    
    // scale from 0->1023 to tft.width
    //p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.x = tft.width()-map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
    //p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);
    if(p.y > BTN_ROW1 && p.y < (BTN_ROW1+BTN_HEIGHT)){
      if(p.x > 15 && p.x <70){
        pressButton(NEXTTRACK_BTN);
      }  
      else if(p.x > 90 && p.x < 145){
        pressButton(PLAYPAUSE_BTN);
      }
      else if(p.x > 165 && p.x < 220){
        pressButton(PREVTRACK_BTN);
      }
    } else if(p.y > BTN_ROW2 && p.y < (BTN_ROW2+BTN_HEIGHT)){
      if(p.x > 135 && p.x < 190){
        pressButton(VOLUME_DN_BTN);
      }  
      else if(p.x > 55 && p.x < 110){
        pressButton(VOLUME_UP_BTN);
      }
    }
  } else {
    // we are getting p.z == 0 while holding down the button, 
    // make sure we have really let up on the button.
    ++numZerosAfterPress;
    if (numZerosAfterPress > ZERO_CHECK) {
      checkReleaseButton();
    }
  }
}

void checkReleaseButton() {
  
  if (buttonPressedPin != 0 && buttonPressedDuration >= BUTTON_PRESSED_DURATION)
  {
    Serial.print("Button Released:");
    Serial.println(buttonPressedPin);
    //Serial.println(numZerosAfterPress);
    digitalWrite(buttonPressedPin, HIGH);
    buttonPressedPin = 0;
    buttonPressedDuration = 0;
  }
}

void pressButton(int pinNo) {
  if (buttonPressedPin != 0)
    return;

  Serial.print("Button Pressed: ");
  Serial.println(pinNo);
  buttonPressedPin = pinNo;
  buttonPressedDuration = 0;
  digitalWrite(pinNo, LOW);
}

#define COLORWIPE_WAIT  50
uint32_t colorWipeColors[] = { strip.Color(255, 0, 0), strip.Color(0, 255, 0), strip.Color(0, 0, 255), strip.Color(0, 0, 0, 255) };
int colorWipeCurrentPixel = 0;
int colorWipeColorIndex = 0;
elapsedMillis colorWipeWaitDuration;

#define WHITE_OVER_RAINBOW_DELAY 20
#define WHITE_OVER_RAINBOW_NUM_LOOPS 3

elapsedMillis whiteOverRainbowDuration = 0;
int whiteOverRainbowLoopCtr = 0;
int whiteOverRainbowLoopNum = 0;
int head = strip.numPixels() - 2;
int tail = 0;
unsigned long lastTime = 0;

#define PULSE_WHITE_DELAY 5
int pulseWhiteStageOne = 0;
int pulseWhiteStageTwo = 255;
elapsedMillis pulseWhiteWait = 0;

#define FULL_WHITE_DELAY 2000
elapsedMillis fullWhiteWait = 0;
bool fullWhiteStarted = false;

void processLights() {
  // Some example procedures showing how to display to the pixels:
  colorWipe();

  if (colorWipeColorIndex >= 4 && whiteOverRainbowLoopNum < WHITE_OVER_RAINBOW_NUM_LOOPS) {
    whiteOverRainbow(75);
  }

  
  if (colorWipeColorIndex >= 4 && whiteOverRainbowLoopNum >= WHITE_OVER_RAINBOW_NUM_LOOPS) {
    pulseWhite(); 
  }
  
  if (colorWipeColorIndex == 4 && whiteOverRainbowLoopNum == WHITE_OVER_RAINBOW_NUM_LOOPS && pulseWhiteStageOne == 256 && pulseWhiteStageTwo == -1 && !fullWhiteStarted) {
    fullWhite();
    fullWhiteWait = 0;
    fullWhiteStarted = true;
    // Serial.println("Started Full White");
  }
  
  // Serial.println("Rainbow start");
  // rainbowFade2White(3,3,1);
  // Serial.println("Rainbow end");
  
  // reset them all...
  if (colorWipeColorIndex == 4 && 
      whiteOverRainbowLoopNum == WHITE_OVER_RAINBOW_NUM_LOOPS && 
      pulseWhiteStageOne == 256 && pulseWhiteStageTwo == -1 &&
      fullWhiteStarted && fullWhiteWait > FULL_WHITE_DELAY) {
        
    // Serial.println("Resetting...");
    colorWipeColorIndex = 0;
    colorWipeCurrentPixel = 0;
    colorWipeWaitDuration = 0;

    whiteOverRainbowDuration = 0;
    whiteOverRainbowLoopCtr = 0;
    whiteOverRainbowLoopNum = 0;
    head = strip.numPixels() - 2;
    tail = 0;
    lastTime = 0;

    pulseWhiteStageOne = 0;
    pulseWhiteStageTwo = 255;
    
    fullWhiteStarted = false;
    fullWhiteWait = 0;
  }
}

// Fill the dots one after the other with a color
void colorWipe() {
  // increment color
  if (colorWipeCurrentPixel >= strip.numPixels())
  {
    colorWipeCurrentPixel = 0;
    ++colorWipeColorIndex;
    //Serial.print("Setting next color index: ");
    //Serial.println(colorWipeColorIndex);
  }

  if (colorWipeColorIndex >= 4) {
    return;
  }

  // once color has displayed long enough, set next one.
  if (colorWipeWaitDuration >= COLORWIPE_WAIT)
  {
    strip.setPixelColor(colorWipeCurrentPixel, colorWipeColors[colorWipeColorIndex]);
    strip.show();
    //Serial.print("Setting pixel #: ");
    //Serial.println(colorWipeCurrentPixel);
    ++colorWipeCurrentPixel;
    colorWipeWaitDuration = 0;
  }
}

void pulseWhite() {
  if (pulseWhiteWait < PULSE_WHITE_DELAY)
    return;
  
  if (pulseWhiteStageOne < 256) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[pulseWhiteStageOne] ) );
    }
    pulseWhiteWait = 0;
    strip.show();
    pulseWhiteStageOne++;
  } else if (pulseWhiteStageTwo >= 0) {
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[pulseWhiteStageTwo] ) );
    }
    pulseWhiteWait = 0;
    strip.show();
    pulseWhiteStageTwo--;
  }
}

void rainbowFade2White(uint8_t wait, int rainbowLoops, int whiteLoops) {
  float fadeMax = 100.0;
  int fadeVal = 0;
  uint32_t wheelVal;
  int redVal, greenVal, blueVal;

  for(int k = 0 ; k < rainbowLoops ; k ++){
    
    for(int j=0; j<256; j++) { // 5 cycles of all colors on wheel

      for(int i=0; i< strip.numPixels(); i++) {

        wheelVal = Wheel(((i * 256 / strip.numPixels()) + j) & 255);

        redVal = red(wheelVal) * float(fadeVal/fadeMax);
        greenVal = green(wheelVal) * float(fadeVal/fadeMax);
        blueVal = blue(wheelVal) * float(fadeVal/fadeMax);

        strip.setPixelColor( i, strip.Color( redVal, greenVal, blueVal ) );

      }

      //First loop, fade in!
      if(k == 0 && fadeVal < fadeMax-1) {
          fadeVal++;
      }

      //Last loop, fade out!
      else if(k == rainbowLoops - 1 && j > 255 - fadeMax ){
          fadeVal--;
      }

        strip.show();
        delay(wait);
    }
  
  }
  
  delay(500);


  for(int k = 0 ; k < whiteLoops ; k ++){

    for(int j = 0; j < 256 ; j++){

        for(uint16_t i=0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[j] ) );
          }
          strip.show();
        }

        delay(2000);
    for(int j = 255; j >= 0 ; j--){

        for(uint16_t i=0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, strip.Color(0,0,0, neopix_gamma[j] ) );
          }
          strip.show();
        }
  }

  delay(500);


}

void whiteOverRainbow(uint8_t whiteSpeed) {

  if (whiteOverRainbowDuration <= WHITE_OVER_RAINBOW_DELAY)
    return;

  if (whiteOverRainbowLoopCtr >= 256)
    whiteOverRainbowLoopCtr = 0;

  // Serial.print(whiteOverRainbowLoopNum);
  // Serial.print(" ");
  // Serial.print(whiteOverRainbowLoopCtr);
  // Serial.print(" ");
  // Serial.print(head);
  // Serial.print(" ");
  // Serial.println(tail);

  int j = whiteOverRainbowLoopCtr++;
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    if((i >= tail && i <= head) || (tail > head && i >= tail) || (tail > head && i <= head) ){
      strip.setPixelColor(i, strip.Color(0,0,0, 255 ) );
      // Serial.print("there: ");
    }
    else{
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
      // Serial.print("here: ");
    }
    
    // Serial.println(i);
  }

  if(millis() - lastTime > whiteSpeed) {
    head++;
    tail++;
    if(head == strip.numPixels()){
      whiteOverRainbowLoopNum++;
    }
    lastTime = millis();
  }

  head%=strip.numPixels();
  tail%=strip.numPixels();

  strip.show();
  whiteOverRainbowDuration = 0;
}

void fullWhite() {
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0,0,0, 255 ) );
    }
      strip.show();
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256 * 5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

uint8_t red(uint32_t c) {
  return (c >> 16);
}
uint8_t green(uint32_t c) {
  return (c >> 8);
}
uint8_t blue(uint32_t c) {
  return (c);
}

// background image
