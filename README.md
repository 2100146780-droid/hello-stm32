# hello-stm32

我的第一个 Git 练习仓库，用来走通「本地编写 → add → commit → push → GitHub」的完整流程

之后会在这个仓库里放 STM32 入门代码：点灯、按键消抖、串口打印。

## 环境记录

| 项目 | 值 |
|---|---|
| MCU 目标板 | STM32F103C8T6 |
| IDE | Keil MDK5（江协科技教程配套） |
| Git | 2.56.0 (Windows) |
| 连接方式 | SSH over 443 端口 |

## 目录结构

```
hello-stm32/
├── README.md          本说明文件
├── .gitignore         忽略编译产物与 conda 环境
├── notes/             学习笔记
├── STM32F103C8T6引脚定义-江协科技.png   引脚图速查
└── src/               全部代码
    ├── README.md      ★ 逐工程代码说明与问题分析（先看这个）
    ├── hello.c        C 语言入门练习
    ├── 1_project_temple/  工程模板
    ├── 2_led/             点亮 LED
    ├── 4_button/          按键输入
    ├── 5_OLED/            OLED 显示
    ├── 6.interrupt/       中断
    ├── 7_TIM_LED/         定时器
    ├── (9_OLED+Key+TIM)/  OLED+按键+定时器综合
    ├── 10_PWM/            PWM 呼吸灯
    ├── 12_Motor/          电机驱动
    ├── 13_chuankou/       串口通信
    ├── 14_ADC/            ADC 采集
    ├── 15-TCRT5000+servo+HC-SR04+OLED/  循迹+舵机+超声波+OLED 综合
    └── 16_HC-SR04/        超声波测距
```

> 注：`.3_shumaguan`、`.8_TIM_LED+AI(S)`、`.11_Servo_motor` 三个工程因文件名以点开头，在 Windows 中默认隐藏；数码管、定时器进阶、舵机等内容请开启「显示隐藏文件」后查看。

## 学习日志

- 2026-11-07 完成 Git/GitHub 环境搭建，SSH 连接测试通过。
- 2026-11-07 第一次自己动手提交，搞懂了 add / commit / push 的区别。
- 2026-11-07 移入了之前的代码


## 代码说明

每个工程在练习什么、关键代码在哪、有哪些可以改进的地方，都整理在 👉 [src/README.md](src/README.md)
