#ifndef __audio_output_pwm_h__
#define __audio_output_pwm_h__

#include <AudioOutput.h>
#include "pwm_audio.h"
#define CACHED_COUNT    (32)       // 32, 64, 96 is good for test, 128 will make noise. why???
typedef int16_t pwm_audio_t;
#define sample2pwm(s)     Amplify(s)
#define TIMER_BITS        LEDC_TIMER_10_BIT
#define AUDIO_BITS        PWM_AUDIO_16BITS

class AudioOutputPWM : public AudioOutput {
protected:
  static bool _inited;
  int8_t _gpio_left, _gpio_right;
  pwm_audio_t _cached_samples[CACHED_COUNT];
  int32_t _cached_count;
public:
  AudioOutputPWM(const int8_t gpio_left, const int8_t gpio_right=-1){
    hertz = 44100;
    bps = 16;
    channels = 2;
    _cached_count = 0;
    _gpio_left = gpio_left;
    _gpio_right = gpio_right;
    // _inited = false;
  }
  ~AudioOutputPWM(){};
  virtual bool begin() {
    if(!_inited){
      _inited = true;
      pwm_audio_config_t pac;
      pac.duty_resolution = TIMER_BITS;
      pac.gpio_num_left = _gpio_left;
      pac.ledc_channel_left = LEDC_CHANNEL_0;
      pac.gpio_num_right = _gpio_right;
      pac.ledc_channel_right = LEDC_CHANNEL_1;
      pac.ledc_timer_sel = LEDC_TIMER_0;
      pac.tg_num = TIMER_GROUP_0;
      pac.timer_num = TIMER_0;
      pac.ringbuf_len = 4096;
      pwm_audio_init(&pac);
      // Serial.printf("sample rate : %d, bps: %d, channels: %d\n", hertz, bps, channels);
    }
    pwm_audio_set_param(hertz, AUDIO_BITS, channels);
    pwm_audio_start();
    Serial.printf("AudioOutputPWM.begin: %dHz \n", hertz);    
    return true;
  }
  virtual bool SetRate(int hz) { 
    esp_err_t err = ESP_OK;
    if(hertz != hz){
      hertz = hz;
      pwm_audio_stop();
      err = pwm_audio_set_sample_rate(hertz);
      pwm_audio_start();
      Serial.printf("AudioOutputPWM.SetRate: %dHz, err: %d\n", hertz, err);
    }
    return true;
  }
  virtual bool ConsumeSample(int16_t sample[2]) {
    size_t cnt = 0;
    bool  ok = false;
    if(_cached_count >= CACHED_COUNT){
      pwm_audio_write((uint8_t*)_cached_samples, sizeof(pwm_audio_t) * _cached_count, &cnt, 0);
      if(cnt > 0){
        _cached_count -= (cnt / sizeof(pwm_audio_t));
      }
    }
    if(channels == 2){
      if(_cached_count + 1 < CACHED_COUNT){
        if(_gpio_right == -1){
          int32_t ttl = sample[LEFTCHANNEL] + sample[RIGHTCHANNEL];
          sample[LEFTCHANNEL] = sample[RIGHTCHANNEL] = (ttl>>1) & 0xffff;
        }
        _cached_samples[_cached_count] = sample2pwm(sample[0]);
        _cached_count++;
        _cached_samples[_cached_count] = sample2pwm(sample[1]);
        _cached_count++;
        ok = true;
      }
    }else{
      if(_cached_count < CACHED_COUNT){
        _cached_samples[_cached_count] = sample2pwm(sample[0]);
        _cached_count++;
        ok = true;
      }
    }
    return ok;
  }
  virtual bool stop() {
    pwm_audio_stop();
    /*
    if(_inited){
      pwm_audio_deinit();
      _inited = false;
    }
    */
    return true;
  }
};
bool AudioOutputPWM::_inited = false;

#endif // __audio_output_pwm_h__
