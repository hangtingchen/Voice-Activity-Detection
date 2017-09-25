# Voice-Activity-Detection
A project for VAD. It is a homework project for ASR

------------------

## 文件

你需要使用BasicAudioToolBox的相关库，包括fft,hmath,sigProcess和WAVE

| 文件        | 主要用于         |
| ------------- |:-------------:|
| hmath.c & hmath.h | 基础数学库，包含了向量和矩阵的生成、操作和解析函数 |
| fft.c & fft.h | 快速傅里叶变换，其和sigProcess中的FFT函数对接 |
| sigProcess.c &sigProcess.h | 基本的信号处理函数 |
| WAVE.c & WAVE.h | 读取WAVE文件 |
| main.c | VAD检测 |
| PHONE_001.wav | 用于DEMO的音频文件 |

## 安装
将文件中的头文件和源文件加入Visual Studio中，选择相应编译器，生成解决方案。

可能需要在预处理器定义中加入`_CRT_SECURE_NO_WARNINGS`。

## 使用
### 使用方法
```Shell
VAD.exe <需要被检测的WAVE文件> <储存剪切后的audio的文件夹位置>
#example:指定被检测的WAVE文件以及储存剪切后文件的位置
VAD.exe PHONE_001.wav ~/Documents/VAD/audio
#example:运行默认参数，使用PHONE_001.wav，储存在当前文件夹下
VAD.exe
```

### 示例
运行截图
![example1](https://github.com/hangtinghen/Voice-Activity-Detection/blob/master/Pic/1.PNG)
结果
![example2](https://github.com/hangtinghen/Voice-Activity-Detection/blob/master/Pic/result1.PNG)

## 程序概述

1. 打开原始WAVE文件
读取WAVE文件的头和数据块

2. 分帧
取帧长为25ms，帧移为10ms。分帧时采用Hamming Window，并对语音信号做预加重处理，预加重系数为0.97 。

3. 计算平均过零率和短时平均能量
针对每一帧，计算平均过零率和短时平均能量

4. 端点识别
+ 根据信号特点设置高能限MH,低能限ML,无声过零率均值Zs
+ 截取音频中高于高能限MH的部分，判断为浊音
+ 从高能限MH扩展至低能限ML
+ 在扩展至3倍无声过零率均值Zs

5. 打印语音段并且输出相应的WAVE文件
