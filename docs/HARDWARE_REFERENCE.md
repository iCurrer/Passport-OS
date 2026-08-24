# Passport OS V2 — 硬件参考（HARDWARE_REFERENCE）

> 本文是 Passport OS V2 改造期间必须遵守的**硬件事实清单**。
> 详细分析与故障定位见相邻的 `docs/AI_HARDWARE_DEVELOPMENT_GUIDE.md`（更完整，含逐寄存器说明）。
> 本文只保留：板子的“是什么”、“哪些绝不能碰”、以及 V2 改造必须核对的内存/线程/电源要点。
>
> 信息优先级（冲突时后者必须让位给前者）：
> **原理图/PCB/实测 > `components/bsp/include/bsp_pins.h`（单一事实来源）> BSP 实现与本文件 > README/apps。**

---

## 1. 板卡总览

| 项 | 值 |
| --- | --- |
| MCU | ESP32-C3，8 MB Flash，**无 PSRAM（仅内部 RAM）** |
| SDK | ESP-IDF 5.5.x（本机 5.5.5） |
| 屏幕 | ST7789P3，240×320，RGB565，SPI2 @40MHz，mode 0，竖屏 |
| 触摸 | 无（三按键导航） |
| 按键 | UP/DOWN/OK 共用一个 ADC 引脚，电阻分压区分 |
| 音频 | ES8311 codec，I2S0 全双工（可缺省） |
| 电池 | CW2017 电量计，共享 I2C0 地址 0x63（可缺省） |
| 日志 | USB Serial/JTAG（原生 USB GPIO18/19） |
| 背光 | GPIO21，LEDC 5kHz/10bit |

---

## 2. 引脚表（来源 `bsp_pins.h`，改硬件只改这一个文件）

| GPIO | 功能 | 外设 | 关键约束 |
| ---: | --- | --- | --- |
| 0 | 三键公共 ADC 节点 | ADC1_CH0 | 外部 **10kΩ 上拉**；启动 strapping 相关 |
| 1 | LCD CS | SPI 输出 | ST7789 片选 |
| 2 | I2S DOUT | 输出 | MCU→codec 播放 |
| 3 | I2S WS | 输出 | MCU 为 master |
| 4 | I2S DIN | 输入 | codec→MCU 录音 |
| 5 | I2S BCLK | 输出 | 与收发共用 |
| 6 | I2S MCLK | 输出 | codec 需要 MCLK |
| 7 | I2C SCL | 双向开漏 | ES8311 与 CW2017 共用 I2C0 |
| 8 | LCD SCLK | SPI 输出 | SPI2 40MHz |
| 9 | LCD MOSI | SPI 输出 | 无 MISO，不能读屏 |
| 10 | I2C SDA | 双向开漏 | 软件内上拉，硬件仍应有外部上拉 |
| 18/19 | USB Serial/JTAG | USB | **控制台占用，不要改作普通 GPIO** |
| 20 | LCD DC | 输出 | 命令/数据选择 |
| 21 | LCD 背光 PWM | LEDC | **与 UART0 默认 TX 冲突，控制台不可切回 UART0** |

> LCD RST 与功放使能均为 `-1`：LCD 走 SWRESET 软复位；功放视为常通。已占用 GPIO 为 0–10、18–21；其余号码在无原理图确认前**不得臆断可用**。

---

## 3. 显示与 LVGL（绝不能乱改的项）

- **分辨率固定 240×320**，RGB565，`BSP_LCD_INVERT_COLOR=1`（反色，换屏负片时反向）。
- **屏初始化序列来自面板参考例程**（`bsp_display.c` 的 porch/power/gamma 表），非通用 ST7789 值，**不得随意修改屏幕初始化**（技能/产品红线）。
- LVGL `swap_bytes=true` 必要；ManDCTL 由 LVGL 端口注册显示时重发，底层 mirror 不要再与 rotation 竞争。
- LVGL 显示缓冲 `240×20` 单缓冲 DMA ≈9.6KB；**不要擅自改大行数双缓冲**（C3 无 PSRAM，会把 I2S DMA 挤到 NO_MEM）。
- `sdkconfig.defaults` LVGL 内部池 48KB（AGENTS 提到 24KB 的旧值已更新为 48KB，以 `sdkconfig.defaults` 为准），加内存必须联合评估最大连续堆。
- **LVGL 非线程安全**：非 LVGL 任务访问 `lv_*` 前后必须 `bsp_lvgl_lock()`/`bsp_lvgl_unlock()`；锁失败安全退出，加锁路径每条都解锁。

### 字库红线（历史踩坑，极易复发）
- 用与 LVGL 9.5 匹配的 `lv_font_conv` 生成；确认 `bitmap_format=1`(COMPRESSED)。
- **必须带 ASCII `0x20–0x7E`**，否则中文正常但英文/数字缺失。
- 中文字库是**静态全量 GB2312**，占用大 Flash（分区表 factory=4MB）；更换字体前先在板上验证中文/英文/数字字形（产品红线）。

---

## 4. 按键（ADC 三键）

- GPIO0 外部 10kΩ 上拉；按下分别走 0Ω/1kΩ/2.2kΩ 分压。电压窗口定义在 `bsp_pins.h` 的 `BSP_BTN_MV_TABLE`。
- **绝不能用 ESP32-C3 内部上拉**（约45kΩ且温漂大，三档会重叠）。
- 整个系统是**单 ADC1 oneshot unit**，BSP、`iot_button`、`bsp_button_read_mv()` 共用；**不得再建第二个 ADC1 unit**（`adc1 already in use`）。
- 事件：PRESS/CLICK/DOUBLE/LONG。V2 主要消费 **CLICK**；长按用于快捷返回/状态切换。
- 回调运行在 button 组件任务，**不能阻塞**，不能做重 UI/录音/播放。

---

## 5. 共享 I2C 红线

- **I2C0 上绝不能再创建第二条 bus/port 总线**（会解绑现有 SDA/SCL 使双芯片失联）。
- ES8311 控制接口由 `esp_codec_dev` 管理，地址 API 要 `0x18 << 1`（8 位形式）；其他 API 用 7 位 `0x18`。
- CW2017：7 位 `0x63`，100kHz，读 VERSION 确认在位；不应答则电量降级 `--%`，整机继续跑。
- `bsp_i2c_scan()` 用 `i2c_master_probe()`，别自建临时总线。

---

## 6. 音频（ES8311，涉及才看）

- I2S0 全双工，MCU 为 master，16kHz/16bit/单声道实测通路。
- **格式变更必须先 close 再 open**（`esp_codec_dev_open` 对已打开设备不重新配置采样率）；此逻辑已封装在 `bsp_audio_set_format`，不得删。
- 不手写 ES8311 时钟分频寄存器；`no_dac_ref=true` 是单声道录音必需，不得改 false。
- `bsp_audio_read/write` 是**阻塞调用**，绝不能放按键回调/LVGL 任务。
- 录音长缓冲约 96KB，是当前最大瞬时堆分配；新增长录音须流式/外部存储。

---

## 7. 电源与 Flash

- 8MB Flash 固定（`CONFIG_ESPTOOLPY_FLASHSIZE_8MB`）；实机探测非 8MB 视为硬件异常，不把默认值降为 4MB。
- 控制台 USB Serial/JTAG；**不得切回 UART0**（TX GPIO21 与背光冲突）。
- 分区表自定义：`nvs 0x6000` + `phy_init` + `factory 4MB`。改动分区表要重新评估 NVS/固件空间。
- 现有功耗机制：默认 7 分钟无操作深度睡眠、GPIO0 低电平唤醒、开机 800ms 忽略按键、深睡前提交 NVS + 关背光 + LCD DISP_OFF。
- V2 目标：**默认 BLE OFF / Wi-Fi OFF**，仅设置内连手机同步时短暂开 BLE，同步完即关并深睡。

---

## 8. V2 改造必须持续核对的内存/线程规则

1. 无 PSRAM：LVGL buffer、音频、头像（80×80 RGB565 = 12,800B）、页面对象都占内部 RAM，先看 build 内存报告与运行时最大的 free 连续块。
2. LVGL 调用必须锁定；页面退出先停定时器/任务再删对象并置空静态指针。
3. 不要把阻塞式硬件调用放进按键/LVGL 上下文。
4. 保持 `main → ui → bsp` 单向依赖；新驱动放 BSP，新绘制原语放 `components/ui`。

---

## 9. 尚未取得硬件证据（待实测/待用户提供）

- 板卡修订号、原理图/PCB/BOM。
- LCD 模组完整料号与初始化来源。
- 电池型号/容量、CW2017 自定义 profile（当前用芯片自带 Li-Poly）。
- 充电/电源路径、USB 检测、扬声器/麦克风参数、功放极性、I2C 外部上拉值。
- 各 GPIO 未用引脚的物理连接。

> 在无硬件证据时，可安全开发 BSP 已覆盖的功能；涉及**新增低功耗/电源/未用引脚复用/音频功率/电池精度**时，必须先请求硬件证据。