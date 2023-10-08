#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

class Screen{
private:
  /* data */
  uint8_t wheelval;
  uint32_t ts_render;
  MatrixPanel_I2S_DMA *dma_display;
  uint16_t colorWheel(uint8_t pos);
  void drawText(int colorWheelOffset);
  void drawBands(const uint8_t audio_bands[16], int colorWheelOffset);
public:
  Screen(/* args */);
  ~Screen();
  void setup();
  void loop(const uint8_t audio_bands[16]);
};
