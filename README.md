# BLMusicPanel

使用ESP32-S3模组，播放bilibili.com音频流带频谱显示的播放器演示程序。

## 演示视频
 - ## 正在制作 ...
 
## 背景
 - 使用ESP32的PWM音频驱动方式，播放音频，无需I2S解码芯片，音质还行吧。。。
 - 使用了DMA方式来驱动HUB75的全彩单元板，显示功能CPU消耗低，余下的算力可以用来做FFT频谱分析

## 特点

- 驱动板成本低：主要元器件，一块ESP32模组，一个FM8002A功放芯片，一个iPhone7的扬声器。
- 全彩单元板闲鱼上有大量，非常适合DIY爱好者。
- 在线播放B站音频，资源丰富。

## 安装

### 源码安装

1. 修改src/wifi_config.h中定义WIFI网络为自己家的网络，再使用vscode+platformio编译刷入固件到开发板即可

## 查看日志
  - 可以通过串口输出查看运行时的各种调试信息,还有重置的数字码也可以从串口输出中查看。

## 贡献

## 致谢
  - https://github.com/earlephilhower/ESP8266Audio
  - https://github.com/noolua/Adafruit-GFX-Library.git
  - https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA.git

## 许可证
  - GPL-3.0 license