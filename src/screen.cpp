#include "screen.h"

#include "uufont.h"
#include "Fonts/Dinkie9_10pt.h"

#ifndef LED_PIN_E
#define LED_PIN_E -1
#endif
#ifdef CONFIG_IDF_TARGET_ESP32S3
#define R1_PIN  4
#define G1_PIN  5
#define B1_PIN  6
#define R2_PIN  7
#define G2_PIN  15
#define B2_PIN  16
#define A_PIN   17
#define B_PIN   18
#define C_PIN   8
#define D_PIN   9
#define E_PIN   LED_PIN_E     // JMP 10
#define CLK_PIN 11
#define LAT_PIN 12
#define OE_PIN  13
#else
#define R1_PIN  13
#define G1_PIN  33
#define B1_PIN  12
#define R2_PIN  14
#define G2_PIN  32
#define B2_PIN  27
#define A_PIN   26
#define B_PIN   19
#define C_PIN   4
#define D_PIN   18
#define E_PIN   LED_PIN_E     // JMP 21
#define CLK_PIN 2
#define LAT_PIN 5
#define OE_PIN  15
#endif

#define PANEL_RES_X 64     // Number of pixels wide of each INDIVIDUAL panel module. 
#define PANEL_RES_Y 32     // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 1      // Total number of panels chained one to another


uint16_t myBLACK = MatrixPanel_I2S_DMA::color565(0, 0, 0);
uint16_t myWHITE = MatrixPanel_I2S_DMA::color565(255, 255, 255);
uint16_t myRED = MatrixPanel_I2S_DMA::color565(255, 0, 0);
uint16_t myGREEN = MatrixPanel_I2S_DMA::color565(0, 255, 0);
uint16_t myBLUE = MatrixPanel_I2S_DMA::color565(0, 0, 255);



// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
// From: https://gist.github.com/davidegironi/3144efdc6d67e5df55438cc3cba613c8
uint16_t Screen::colorWheel(uint8_t pos) {
  if(pos < 85) {
    return dma_display->color565(pos * 3, 255 - pos * 3, 0);
  } else if(pos < 170) {
    pos -= 85;
    return dma_display->color565(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return dma_display->color565(0, pos * 3, 255 - pos * 3);
  }
}

void Screen::drawText(int colorWheelOffset)
{
  
  // draw text with a rotating colour
  // dma_display->setTextSize(1);     // size 1 == 8 pixels high
  // dma_display->setTextWrap(false); // Don't wrap at end of line - will do ourselves

  // uint8_t w = 0;
  // dma_display->setCursor(4, 10);
  // for (w=9; w<18; w++) {
  //   dma_display->setTextColor(colorWheel((w*32)+colorWheelOffset));
  //   dma_display->print("*");
  // }
  
  // dma_display->setCursor(4, 18);
  // dma_display->setTextColor(dma_display->color444(15,15,15));
  // dma_display->setUUFont(&Dinkie9demo_10pt);
  // dma_display->print("hi! 你好呀!");
  // dma_display->setUUFont(NULL);
}

void Screen::drawBands(const uint8_t audio_bands[16], int colorWheelOffset){
  int16_t x = 0, y = 8, w = 16, h = 8, bw = 4;
  // dma_display->fillRect(x, y, w, h, dma_display->color333(0, 0, 0));
  for(int i=0; i < w; i++){
    int v = audio_bands[i];
    dma_display->drawRect(x+(i*bw), y+h-v, bw, v+1, colorWheel((i*32)+colorWheelOffset));
    if (v > 0) dma_display->drawRect(x+(i*bw), y+h, bw, v+1, colorWheel((i*32)+colorWheelOffset));
    // dma_display->drawLine(x+i, y+h, x+i, y+h-v, colorWheel((i*32)+colorWheelOffset));
  }
}

Screen::Screen(/* args */){
  dma_display = nullptr;
  ts_render = 0;
}

Screen::~Screen()
{
}

void Screen::setup(){
  // Module configuration
  HUB75_I2S_CFG::i2s_pins i2s_pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,   // module width
    PANEL_RES_Y,   // module height
    PANEL_CHAIN,   // Chain length
    i2s_pins,      // i2s_pins
    HUB75_I2S_CFG::SHIFTREG, // shift_driver
    true,          // double buffer
    HUB75_I2S_CFG::HZ_15M, // clk speed
    2, true, 180, 8
  );

  // Display Setup
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(32); //0-255
  dma_display->clearScreen();  
  // fill the screen with 'black'
  dma_display->fillScreen(dma_display->color444(0, 0, 0));
}

void Screen::loop(const uint8_t audio_bands[16]){
  uint32_t ts_now = millis();
  if (ts_render < ts_now){
    ts_render = ts_now + 50;
    dma_display->clearScreen();
    dma_display->fillScreen(dma_display->color444(0, 0, 0));
    drawText(wheelval);
    drawBands(audio_bands, wheelval);
    wheelval += 16;
    dma_display->flipDMABuffer();
  }
}
