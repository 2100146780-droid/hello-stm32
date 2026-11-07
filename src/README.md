# 代码说明与逐工程分析

本目录是江协科技《STM32 入门 100 步》配套工程，按学习顺序编号。这份文档记录**每个工程在练什么、关键代码在哪里、以及代码里值得改进的地方**。

> 分析日期：2026-11-07　｜　编译环境：Keil MDK5 + 标准外设库（StdPeriph）

---

## 一、总览

| 工程 | 练习主题 | 关键外设 | 代码量 | 状态 |
|---|---|---|---|---|
| `1_project_temple` | 工程模板 | — | 12 行 | 骨架，只有空 `while(1)` |
| `2_led` | GPIO 输出点灯 | GPIOA PA0 | 21 行 | 可用 |
| `.3_shumaguan` | 数码管静态显示 | GPIOA PA0-PA7、GPIOB | 27 行 | 段码直接写死，未做动态扫描 |
| `4_button` | 按键输入 | GPIOB PB11 上拉 + 延时消抖 | 23 行 | 可用，但会阻塞 |
| `5_OLED` | OLED 显示 | PB8/PB9 软件 I2C | 35 行 | 可用 |
| `6.interrupt` | 外部中断 | PA0 → EXTI0 → NVIC | 19 行 | 可用（主循环为空） |
| `7_TIM_LED` | 定时器中断定时 | TIM2 + NVIC | 113 行 | 可用，有 2 处隐患 |
| `.8_TIM_LED+AI(S)` | 定时周期动态可调 | TIM2 改 ARR | 94 行 | 可用，写法规范 |
| `(9_OLED+Key+TIM)` | 秒表（状态机） | TIM2 1ms + OLED + EXTI | 92 行 | 完成度最高的一个 |
| `10_PWM` | **超声波 + 舵机整合** | TIM2_CH3 PWM、TIM3 秒表、EXTI | 335 行 | 目录名不符，实际是测距+舵机 |
| `.11_Servo_motor` | 舵机角度控制 | TIM2_CH3 → PA2，50Hz | 75 行 | 可用 |
| `12_Motor` | 舵机（同 11 的简化版） | 同上 | 134 行 | 大段旧代码被注释保留 |
| `13_chuankou` | 串口收发 | USART1（PA9/PA10）+ 中断 | 64 行 | 只发不收（接收部分全注释） |
| `14_ADC` | ADC + DMA 双通道 | ADC1、DMA1、PA1/PA2 | 74 行 | 靠 Debug 看 `AD_Value[]` |
| `15-TCRT5000+servo+HC-SR04+OLED` | 三传感器综合 | 循迹 + 测距 + 舵机 + OLED | 372 行 | 功能最完整，注释最详细 |
| `16_HC-SR04` | 超声波测距 | EXTI 双边沿 + TIM 秒表 + 串口 | 4 个子工程 | 教学注释版 |
| `hello.c` | C 语言入门 | — | 7 行 | printf 练习 |

---

## 二、逐工程说明

### 1_project_temple — 工程模板
只有 `stm32f10x.h` 和空 `while(1)`。作用是把 Keil 工程的**目录结构、库文件、Define 设置**固定下来，后面每课都从这个模板复制。关键设置：`USE_STDPERIPH_DRIVER, STM32F10X_MD`，Include 路径指向 CMSIS 与 `StdPeriph_Driver\inc`。

### 2_led — GPIO 输出
`LED.c` 里 `LED_Init()` 把 **PA0** 配成推挽输出（`GPIO_Mode_Out_PP`），`main` 里 `GPIO_SetBits(GPIOA, GPIO_Pin_0)` 输出高电平。
> 注意：代码注释写"LED 负极接 GND"，那么输出高电平才是亮——如果你的板子是 LED 正极接 3.3V 的接法，现象会正好相反。

### .3_shumaguan — 数码管
直接对 **PA0-PA7** 和 **PB0/PB1/PB3/PB4** 逐个赋值来点亮段码，把要显示的数字"手算"成 0/1 序列写进代码。
- 这是最原始的静态显示，没有段码表、没有动态扫描。
- 真正的数码管显示需要：段码表（`0x3F` 表示 "0" 等）+ 位选快速轮询（利用视觉暂留）。
- 建议下一次重写成 `uint8_t SegCode[] = {...}` 查表的方式，可读性会好很多。

### 4_button — 按键输入
`Key.c` 的 `Key_GetFlag()` 是**阻塞式**消抖：检测到低电平 → 延时 10ms → `while(引脚==0)` 死等松手 → 再延时 10ms → 返回 1。
- 优点：简单，不会重复触发。
- 缺点：按住不放时主循环会**卡在这里**，其他任务全部停摆。这就是为什么后面要学 `6.interrupt`。

### 5_OLED — OLED 显示
`OLED_Init()` 后调用 `OLED_ShowNum / ShowChar / ShowString` 在 4 行 16 列的坐标上显示内容。这一课建立的 `OLED.c` 是后面所有工程的显示基础，**每个工程都复制了同一份**。

### 6.interrupt — 外部中断
链路是：**PA0 引脚 → AFIO 映射到 EXTI0 → NVIC 使能 → `EXTI0_IRQHandler` 自动执行**。
```c
GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);  // AFIO 把 PA0 接到 EXTI0
EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;      // 下降沿触发
NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);              // 先分组，再配优先级
```
中断服务函数结尾**必须** `EXTI_ClearITPendingBit(EXTI_Line0)`，否则会反复进入中断。

### 7_TIM_LED — 定时器中断
核心是三个寄存器：`PSC`（预分频，决定计数快慢）、`CNT`（当前计数）、`ARR`（数到这里溢出并触发中断）。
```c
TIM_Prescaler = 7200-1;   // 72MHz / 7200 = 10kHz
TIM_Period    = 10000-1;  // 10000 个数 → 1 秒
```
`main` 里按键按下后调用 `TIM2_SetTimerPeriod(0.3f)` 重设 ARR，并配合 `TIM_GenerateEvent(TIM_EventSource_Update)` 让新值立刻生效（否则要等下一次溢出才生效）。

**发现 2 个问题：**
1. `LED_Turn()` 里用 `GPIO_ReadInputDataBit()` 去读一个**输出**引脚。读输入寄存器拿的是引脚真实电平，LED 接了负载后可能读不准，应该用 `GPIO_ReadOutputDataBit()`——`.8_TIM_LED+AI(S)` 里已经改对了。
2. `Key_I_Init()` 把 **PA0 和 PA1 一起**配成了上拉输入，但 `LED_Init()` 才刚把 PA1 配成推挽输出。后配置的覆盖前面的，**PA1 的 LED 会变成输入模式**，靠 ODR 的弱上下拉拉不动 LED。改法：`Key_I_Init` 里只保留 `GPIO_Pin_0`。

### .8_TIM_LED+AI(S) — 定时周期可调
把定时周期封装成 `SetTIM2_Time(float sec)`，用 `sec * 10000 - 1` 算 ARR，并做了 16 位上限 65535 的保护。主循环每 5 秒切换一次 1s / 0.5s / 2s 周期。
- 这是 7_TIM_LED 的改进版，包含越界保护、宏定义常量、`GPIO_ReadOutputDataBit` 正确用法。
- 中断里直接操作寄存器翻转 PA1（没用 `LED_Turn()`），中断执行时间更短，是更好的做法。

### (9_OLED+Key+TIM) — 秒表状态机 ★
整个仓库里**代码组织最好的一个**：
```c
typedef enum { STATE_IDLE, STATE_RUN, STATE_STOP } RunState;  // 三态：空闲/计时/暂停
```
- 按键中断只置 `g_key_flag = 1`，主循环里再做状态切换——**中断只做最少的事**，这是正确的中断设计思想。
- 计时在 `TIM2_IRQHandler` 里 `g_time_ms++`，主循环只负责刷新 OLED，互不干扰。
- 状态切换逻辑：IDLE → RUN → STOP → IDLE（并清零）。可以扩展成"短按/长按"或"秒表/倒计时"两种模式。

### 10_PWM — 实际是「超声波 + 舵机」
> **目录名和内容不符**：这份代码是 HC-SR04 测距 + 舵机指示的整合版，不是 PWM 呼吸灯。

文件头的注释写得很详细，把定时器分工讲清楚了：
| 定时器 | 用途 | 参数 |
|---|---|---|
| TIM2_CH3 → PA2 | 舵机 PWM | 50Hz，ARR = 20000-1，CCR = 500~2500 |
| TIM3 | 测距秒表 | 1MHz（每计数 1 = 1μs），ARR = 65535 |

- 关键设计：**两个功能必须用两个定时器**，因为一个定时器只有一套 PSC/ARR。
- `TEST_SERVO_ONLY` 开关把代码分成"只测舵机扫掠"和"完整测距+指示"两种模式，是很好的调试思路。
- 已知坑：`PWM.c` / `Motor.c` 也会配置 TIM2，同时存在会把舵机信号搞乱，需要从工程里 Remove。

### .11_Servo_motor — 舵机
`Servo_SetAngle(angle)` 把 0~180° 线性映射成 500~2500μs 脉宽，写进 `TIM_SetCompare3(TIM2, pulse)`。
- 舵机原理：**20ms 周期（50Hz），高电平 0.5ms = 0°，2.5ms = 180°**。
- ARR 设为 20000-1 且 PSC=71，使计数频率 1MHz，于是 CCR 的数值直接就是微秒数——这是很巧妙的做法，省掉换算。

### 12_Motor — 舵机（简化版）
和 `.11` 几乎相同的代码，但 `main` 里有一大段旧的点灯+延时逻辑被整块注释掉保留着（第 8~76 行）。
- 建议：这些注释掉的历史代码应该在 git 里查，而不是留在源文件里。这也是用 git 的意义——**删掉的东西不会真的消失**。

### 13_chuankou — 串口
`uart_init()` 初始化 USART1，然后依次试了三种发送方式：
```c
Serial_SendByte()    // 发一个字节（hex 模式看）
Serial_SendArray()   // 发数组，要写长度
Serial_Printf()      // 带格式化的 printf，需要勾选 Use MicroLIB
```
**发现的问题**：`while(1)` 里接收处理逻辑（`Serial_RxFlag` 判断、`strcmp` 比较指令、回发）**全部被注释掉了**，而且注释后 `if` 的大括号变成空块——也就是说这一课**只实现了发送，接收没跑通**。建议恢复接收部分，用 `if(strcmp(Serial_RxPacket,"AAA")==0)` 做指令控制，这是串口最实用的玩法。
> 另外：发送语句放在了 `while(1)` 外面，所以只发一次。注释里自己也写了"发送数据要放到 while1 外面"——其实想让数据持续发送，应该放进去。

### 14_ADC — ADC + DMA
- PA1、PA2 配成 `GPIO_Mode_AIN`（模拟输入，此时 GPIO 功能关闭，防止干扰）。
- 规则组两个通道：`ADC_Channel_1` 排第 1、`ADC_Channel_2` 排第 2，扫描模式 + 连续转换。
- DMA 把 `ADC1->DR` 自动搬到 `AD_Value[2]` 数组，`MemoryInc = Enable`、`HalfWord` 宽度。
- 因为没有串口显示，所以只能**在 Keil Debug 里看 `AD_Value[]` 的值**——这是这一课的局限，`16_HC-SR04` 里的串口打印就是解法。

**编码问题**：这个文件的注释是"UTF-8 被当成 GBK 再存了一次"的双重乱码，在 Keil 和 GitHub 上都显示不正常，建议重写注释。

### 15-TCRT5000+servo+HC-SR04+OLED — 三合一综合 ★
功能最完整的工程，用**舵机角度当"危险程度指针"**：
```
优先级 1：TCRT5000 压黑线        → 180°（警告）
优先级 2：超声波距离 < 10cm      → 180°
优先级 3：距离 10~60cm           → 角度线性 180→0
优先级 4：> 60cm 或测不到        → 0°（安全）
```
- 文件头记录了实测踩到的**供电坑**：舵机接 STM32 板载 5V 只会"吱吱响不转"（电流不足），必须独立 5V 且共地。这条经验比代码本身值钱。
- 接线：TRIG→PA0、ECHO→PA1、舵机信号→PA2、TCRT5000 DO→PA4、OLED SCL/SDA→PB8/PB9。
- 建议：优先级逻辑可以提取成独立函数 `uint8_t CalcAngle(...)`，`main` 会更清爽。

### 16_HC-SR04 — 超声波测距（4 个子工程）
包含 `16_HC-SR04+chuankou`（串口打印）、`16_HC-SR04+OLED`、`16_HC-SR04+OLED +Servo`、`16_HC-SR04+OLED-TB6612`（含电机驱动）四个渐进版本。

测距原理（`+chuankou` 版）：
1. TRIG 拉高 10μs，模块发 8 个 40kHz 脉冲；
2. ECHO 变高 → 记下 TIM 计数器值 `kaishi`；
3. ECHO 变低 → `kuandu = 当前值 - kaishi`（μs）；
4. 距离 = `kuandu × 0.0343 / 2`（声速 0.0343 cm/μs，除以 2 是因为声音来回）。

```c
volatile uint16_t kaishi, kuandu;  // 中断里改、main 里读，必须 volatile
```
> 注释里把"为什么要 volatile"讲清楚了：不加的话编译器会把 main 里的判断优化掉，导致永远等不到结果。这是中断编程最经典的坑。

### hello.c — C 语言热身
`printf("hello, I am learning STM32!\n")`，用来验证"写代码 → 编译 → 运行"这条链路。在 Windows 上需要先装 gcc 才能跑（当前环境未安装）。

---

## 三、横向对比：同一个功能怎么越写越好

| 功能 | 最初版 | 改进版 | 好在哪里 |
|---|---|---|---|
| LED 翻转 | `7_TIM_LED`：`ReadInputDataBit` | `.8`：`ReadOutputDataBit` | 读输出寄存器才是对的 |
| 按键 | `4_button`：`while()` 死等松手 | `6.interrupt`：EXTI 中断 | 不阻塞主循环 |
| 定时 | `7_TIM_LED`：ARR 写死 | `.8`：`SetTIM2_Time(float)` 封装 | 可复用、有越界保护 |
| 中断写法 | 中断里直接干活 | `(9_...)`：中断置标志，主循环处理 | 中断短，响应快 |
| 串口 | `13`：只发不收 | `16+chuankou`：printf 打印测量值 | 能看见数据 |

---

## 四、共性问题与改进清单

1. **源文件编码是 GBK**：在 Keil 里能正常显示中文，但**推到 GitHub 网页上看是乱码**。要彻底解决需统一转 UTF-8（Keil 里 Edit → Configuration → Encoding 改成 UTF-8，另存全部文件），或者接受这个现状——只是别忘了原因。
2. **`LED_Turn()` 用了 `ReadInputDataBit`**（7_TIM_LED、6.interrupt 等早期工程）：改成 `ReadOutputDataBit`。
3. **`Key_I_Init()` 把 PA1 也配成了输入**：会覆盖 LED 的推挽输出配置，只保留 `GPIO_Pin_0`。
4. **`Key_GetFlag()` 阻塞**：长按会卡住主循环，后续统一改用中断版本（`Key_I.c`）。
5. **注释掉的大段死代码**（`12_Motor`、`13_chuankou`）：交给 git 保存历史，源文件里删掉更清爽。
6. **目录名和内容不符**：`10_PWM` 实际是超声波+舵机、`12_Motor` 实际是舵机。建议改名（改名用 `git mv`，历史不会丢）。
7. **`13_chuankou` 接收功能未完成**：建议补上，串口接收是很多项目的基础。
8. **`14_ADC` 注释乱码 + 无显示**：建议重写注释，并把结果用串口或 OLED 输出。

---

## 五、建议的下一步

1. 把 `13_chuankou` 的接收补完，实现"串口输入指令 → 控制 LED/舵机"；
2. 把 `14_ADC` 的采样值通过串口打印出来（复用 `16+chuankou` 的代码）；
3. 给 `.3_shumaguan` 加段码表和动态扫描，显示变化的数字；
4. 尝试把 `15` 的决策逻辑抽成独立函数，做成"循迹小车"的雏形；
5. 每个工程跑通后，在根目录 `README.md` 的学习日志里补一行。
