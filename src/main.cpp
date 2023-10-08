#include <Arduino.h>
#include <WiFi.h>

#include "version.h"
#include "screen.h"
#include "wifi_config.h"

#include "AudioFileSourceHTTPStreamV2.h"
#include "AudioFileSourceBufferEx.h"
#include "AudioGeneratorAAC.h"
#include "AudioOutputPWM.h"

const char* ssid = WIFI_SSID_DEFAULT;
const char* password = WIFI_PWD_DEFAULT;
const char *URLS[] = {
  "http://kkcdn.fadai8.cn/downloads/dukou.aac",
  "http://10.1.2.175:60588/tun?real=a.bilivideo.cn/s/1696747080.3511775&t=1696747080&s=38527a0be52764c889ee9ac1318f7f92&b=BV1KB4y15742&c=813043148&dbg=9527",
  // "http://10.1.2.175:60588/tun?real=a.bilivideo.cn/s/1696690573.7791595&t=1696690573&s=ad7932e8f3c9dc729cdb76a7e12fb0ef&b=BV1R84y1K7on&c=1088603651&dbg=9527",
  // "http://10.1.2.175:60588/tun?real=a.bilivideo.cn/s/1696694007.899149&t=1696694007&s=9074c05b124f0d98d1e33b598719a6ff&b=BV1cP4y1H71r&c=463535571&dbg=9527",
  // "http://10.1.2.175:60588/tun?real=a.bilivideo.cn/s/1696694007.899149&t=1696694007&s=dba91e3e106598d242d8ba8fd67de552&b=BV1YP411974D&c=1139289804&dbg=9527",
  // "http://10.1.2.175:60588/tun?real=a.bilivideo.cn/s/1696662861.0274172&t=1696662861&s=fd5fa6df894ae2c689713bf339dd6d84&b=BV1nK4y1Y7pg&c=223190531&dbg=9527",
};

#define URLS_NUM    (sizeof(URLS)/sizeof(URLS[0])

#define HAVE_USE_MCUMON
#include "mcumon.h"
McuMonitor __monitor;

typedef AudioFileSourceBufferEx       MyAudioFileSourceBuffer;
typedef AudioFileSourceHTTPStreamV2   MyHttpFileSource;
typedef AudioOutputPWM                MyAudioOutput;


#include "esp32_fft.h"
#define FFT_N       1024
#define SAMPLE_FREQ 48000
#define BAND_NUM    16
#define MAX_LEVEL   12
#define REDUCE_NOSIE  4096
static uint8_t __fft_16bytes[BAND_NUM];

float fftResultPink[BAND_NUM] = {1.70,1.71,1.73,1.78,1.68,1.56,1.55,1.63,1.79,1.62,1.80,2.06,2.47,3.35*3,6.83*3,9.55*3};
//{1.70,1.71,1.73,1.78,1.68,1.56,1.55,1.63,1.79,1.62,1.80,2.06,2.47,3.35,6.83,9.55};

class MyAudioOutputEx: public MyAudioOutput{
  protected:
  int32_t _filled_samples;
  float _fft_input[FFT_N], _fft_output[FFT_N], _fft_bins[FFT_N/2], _fft_bands[BAND_NUM];
  ESP_fft *_fft;
public:
  MyAudioOutputEx(int io_l, int io_r=-1):MyAudioOutput(io_l, io_r){
    memset(__fft_16bytes, 0, BAND_NUM);
    _filled_samples = 0;
    _fft = new ESP_fft(FFT_N, SAMPLE_FREQ, FFT_REAL, FFT_FORWARD, _fft_input, _fft_output);
  }
  virtual ~MyAudioOutputEx(){
    if(_fft) delete _fft;
    memset(__fft_16bytes, 0, BAND_NUM);
  }
  float fft_add(int begin, int end){
    float sum = 0.0;
    int i = begin;
    while(i <= end){
      sum += _fft_bins[i++];
    }
    return sum;
  }
  virtual bool ConsumeSample(int16_t sample[2]){
    int32_t ttl = sample[LEFTCHANNEL] + sample[RIGHTCHANNEL];
    int16_t mix = (ttl>>1) & 0xffff;
    _fft_input[_filled_samples++] = (float)mix;
    if(_filled_samples >= FFT_N){
      _filled_samples = 0;
      _fft->removeDC();
      _fft->hammingWindow();
      _fft->execute();
      _fft->complexToMagnitude();
      float max_mag = 0.0, f0;
      for(int i=1; i < FFT_N/2; i++){
        f0 = abs(_fft_output[i]) / 64.0;
        _fft_bins[i] = f0;
        if(f0 > max_mag){
          max_mag = f0;
        }
      }
    /* This FFT post processing is a DIY endeavour. What we really need is someone with sound engineering expertise to do a great job here AND most importantly, that the animations look GREAT as a result.
     *
     * Andrew's updated mapping of 256 bins down to the 16 result bins with Sample Freq = 10240, samples = 512 and some overlap.
     * Based on testing, the lowest/Start frequency is 60 Hz (with bin 3) and a highest/End frequency of 5120 Hz in bin 255.
     * Now, Take the 60Hz and multiply by 1.320367784 to get the next frequency and so on until the end. Then detetermine the bins.
     * End frequency = Start frequency * multiplier ^ 16
     * Multiplier = (End frequency/ Start frequency) ^ 1/16
     * Multiplier = 1.320367784
     */
/*
      _fft_bands[0] = fft_add(3, 4) / 2;
      _fft_bands[1] = fft_add(4, 5) / 2;
      _fft_bands[2] = fft_add(5, 7) / 3;
      _fft_bands[3] = fft_add(7, 10) /  4;
      _fft_bands[4] = fft_add(10,  14) /  5;
      _fft_bands[5] = fft_add(14,  20) /  7;
      _fft_bands[6] = fft_add(20,  27) /  8;
      _fft_bands[7] = fft_add(27,  38) /  12;
      _fft_bands[8] = fft_add(38,  52) /  15;
      _fft_bands[9] = fft_add(52,  72) /  21;
      _fft_bands[10] = fft_add(72,  100) / 29;
      _fft_bands[11] = fft_add(100, 139) / 40;
      _fft_bands[12] = fft_add(139, 193) / 55;
      _fft_bands[13] = fft_add(193, 267) / 75;
      _fft_bands[14] = fft_add(267, 370) / 104;
      _fft_bands[15] = fft_add(370, 511) / 142;
*/
      _fft_bands[0] = fft_add(3, 4) / 2;
      _fft_bands[1] = fft_add(4, 5) / 2;
      _fft_bands[2] = fft_add(4, 6) / 3;
      _fft_bands[3] = fft_add(6, 7) / 2;
      _fft_bands[4] = fft_add(7, 9) / 3;
      _fft_bands[5] = fft_add(9, 11) /  3;
      _fft_bands[6] = fft_add(11,  14) /  4;
      _fft_bands[7] = fft_add(14,  18) /  5;
      _fft_bands[8] = fft_add(18,  23) /  6;
      _fft_bands[9] = fft_add(23,  29) /  7;
      _fft_bands[10] = fft_add(29,  37) /  9;
      _fft_bands[11] = fft_add(37,  47) /  11;
      _fft_bands[12] = fft_add(47,  59) /  13;
      _fft_bands[13] = fft_add(59,  74) /  16;
      _fft_bands[14] = fft_add(74,  94) /  21;
      _fft_bands[15] = fft_add(94,  119) / 26;

      for(int i = 0;i<16; i++){
        uint8_t v = uint8_t(_fft_bands[i] * fftResultPink[i] / REDUCE_NOSIE);
        __fft_16bytes[i] = v < MAX_LEVEL ? v : MAX_LEVEL;
      }
      // Serial.printf("bands: %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n", f16[0], f16[1], f16[2], f16[3], f16[4], f16[5], f16[6], f16[7], f16[8], f16[9], f16[10], f16[11], f16[12], f16[13], f16[14], f16[15]);

      // Serial.printf("max_mag: %f\n", max_mag);

      // Serial.printf("DC component : %f g\n", (_fft_output[0])/FFT_N);  // DC is at [0]
      // Serial.printf("Fundamental Freq : %f Hz\t Mag: %f g\n", _fft->majorPeakFreq(), (_fft->majorPeak()/10000)*2/FFT_N);
    }
    return MyAudioOutput::ConsumeSample(sample);
  }
};


MyHttpFileSource *file = nullptr;
AudioGeneratorAAC *mp3 = nullptr;
MyAudioOutputEx *out = nullptr;
Screen *screen = nullptr;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("version: %s, build: %s\n", BUILD_VERSION, BUILD_TIME);
  screen = new Screen();
  screen->setup();

  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  
  WiFi.begin(ssid, password);

  // Try forever
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("...Connecting to WiFi");
    delay(1000);
  }
  Serial.println("Connected");

  audioLogger = &Serial;
  out = new MyAudioOutputEx(PWMAUDIO_PIN);
  out->SetGain(1.0);
}

void loop() {
  static uint32_t _ts_next_audio = 0, _index_audio = 0;
  __monitor.loop();  
  screen->loop(__fft_16bytes);
  uint32_t ts_now = millis();
  if(mp3){
    if (mp3->isRunning()) {
      if (!mp3->loop()) mp3->stop();
    } else {
      if(mp3){
        delete mp3;
        mp3 = nullptr;
        Serial.printf("MP3 free\n");
      }
      if(file){
        delete file;
        file = nullptr;
        Serial.printf("FILE free\n");
      }
    }
  }else{
    // new audio
    if (_ts_next_audio < ts_now){
      const char *url = URLS[_index_audio++ % URLS_NUM)];
      file = new MyHttpFileSource(url);
      MyAudioFileSourceBuffer *buff = new MyAudioFileSourceBuffer(file, 1024*(256));
      mp3 = new AudioGeneratorAAC();
      mp3->begin(buff, out);
      Serial.println(url);
      _ts_next_audio = ts_now + 5000;
    }
  }
}
