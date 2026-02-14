/***************************************************
  Arduino TFT graphics library targeted at ESP8266
  and ESP32 based boards.

  This is a stand-alone library that contains the
  hardware driver, the graphics functions and the
  proportional fonts.

  The built-in fonts 4, 6, 7 and 8 are Run Length
  Encoded (RLE) to reduce the FLASH footprint.

  Last review/edit by Bodmer: 04/02/22
 ****************************************************/

// 防止字体等内容被多次加载（头文件保护宏）
#ifndef _TFT_eSPIH_
#define _TFT_eSPIH_

// 定义库版本号
#define TFT_ESPI_VERSION "2.5.43"

// 位级功能标志位
// 第0位：启用视口（viewport）功能
#define TFT_ESPI_FEATURES 1

/***************************************************************************************
**                         Section 1: Load required header files
**                         第1节：加载必需的头文件
***************************************************************************************/

// 标准支持库
#include <Arduino.h>       // Arduino核心库
#include <Print.h>         // 打印功能基类
// 非并行8位接口且非RP2040 PIO接口时，加载SPI库
#if !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_INTERFACE)
  #include <SPI.h>         // SPI通信库
#endif

/***************************************************************************************
**                         Section 2: Load library and processor specific header files
**                         第2节：加载库和处理器特定的头文件
***************************************************************************************/
// 包含定义加载的字体、TFT驱动、引脚等配置的头文件
#ifdef CONFIG_TFT_eSPI_ESPIDF  // ESP-IDF环境下
  #include "TFT_config.h"
#endif

// 兼容新旧ESP8266开发板包的架构定义
// 新包使用ARDUINO_ARCH_ESP8266，旧包定义ESP8266
#if defined (ESP8266)
  #ifndef ARDUINO_ARCH_ESP8266
    #define ARDUINO_ARCH_ESP8266
  #endif
#endif

// 支持用户在sketch目录下自定义tft_setup.h配置文件
#if !defined __has_include  // 编译器不支持__has_include时
  #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
    // 警告：编译器不支持__has_include，sketch无法自定义配置
    #warning Compiler does not support __has_include, so sketches cannot define the setup
  #endif
#else
  #if __has_include(<tft_setup.h>)  // 检测是否存在tft_setup.h
    #include <tft_setup.h>          // 加载用户自定义配置
    #ifndef USER_SETUP_LOADED
      #define USER_SETUP_LOADED     // 标记已加载用户配置，防止重复加载
    #endif
  #endif
#endif

// 加载用户配置选择文件（选择不同的硬件配置）
#include <User_Setup_Select.h>

// 处理基于FLASH的存储（如PROGMEM），兼容不同处理器架构
#if defined(ARDUINO_ARCH_RP2040)  // RP2040架构
  // 重定义PROGMEM读取函数（RP2040无专用PROGMEM，直接访问）
  #undef pgm_read_byte
  #define pgm_read_byte(addr)   (*(const unsigned char *)(addr))
  #undef pgm_read_word
  #define pgm_read_word(addr) ({ \
    typeof(addr) _addr = (addr); \
    *(const unsigned short *)(_addr); \
  })
  #undef pgm_read_dword
  #define pgm_read_dword(addr) ({ \
    typeof(addr) _addr = (addr); \
    *(const unsigned long *)(_addr); \
  })
#elif defined(__AVR__)  // AVR架构（如Uno/Nano）
  #include <avr/pgmspace.h>  // AVR的PROGMEM库
#elif defined(ARDUINO_ARCH_ESP8266) || defined(ESP32)  // ESP8266/ESP32
  #include <pgmspace.h>      // ESP系列的PROGMEM库
#else  // 其他架构
  #ifndef PROGMEM
    #define PROGMEM           // 空定义，兼容无PROGMEM的架构
  #endif
#endif

// 加载处理器特定的驱动文件
#if defined(CONFIG_IDF_TARGET_ESP32S3)  // ESP32-S3
  #include "Processors/TFT_eSPI_ESP32_S3.h"
#elif defined(CONFIG_IDF_TARGET_ESP32C3)  // ESP32-C3
  #include "Processors/TFT_eSPI_ESP32_C3.h"
#elif defined (ESP32)  // 通用ESP32
  #include "Processors/TFT_eSPI_ESP32.h"
#elif defined (ARDUINO_ARCH_ESP8266)  // ESP8266
  #include "Processors/TFT_eSPI_ESP8266.h"
#elif defined (STM32)  // STM32
  #include "Processors/TFT_eSPI_STM32.h"
#elif defined(ARDUINO_ARCH_RP2040)  // RP2040（如Raspberry Pi Pico）
  #include "Processors/TFT_eSPI_RP2040.h"
#else  // 通用处理器
  #include "Processors/TFT_eSPI_Generic.h"
  #define GENERIC_PROCESSOR  // 标记通用处理器
#endif

/***************************************************************************************
**                         Section 3: Interface setup
**                         第3节：接口配置
***************************************************************************************/
#ifndef TAB_COLOUR  // 未定义标签颜色时
  #define TAB_COLOUR 0  // 默认标签颜色（ST7735屏专用，现已无效）
#endif

// 未定义SPI频率时，设置默认值20MHz
#ifndef SPI_FREQUENCY
  #define SPI_FREQUENCY  20000000
#endif

// 未定义SPI读取频率时，设置默认值10MHz
#ifndef SPI_READ_FREQUENCY
  #define SPI_READ_FREQUENCY 10000000
#endif

// 部分ST7789屏幕不兼容SPI模式0，默认适配
#ifndef TFT_SPI_MODE
  #if defined(ST7789_DRIVER) || defined(ST7789_2_DRIVER)
    #define TFT_SPI_MODE SPI_MODE3  // ST7789用模式3
  #else
    #define TFT_SPI_MODE SPI_MODE0  // 其他驱动用模式0
  #endif
#endif

// 未定义XPT2046触摸芯片SPI频率时，设置默认值2.5MHz
#ifndef SPI_TOUCH_FREQUENCY
  #define SPI_TOUCH_FREQUENCY  2500000
#endif

// 默认启用SPI忙检测
#ifndef SPI_BUSY_CHECK
  #define SPI_BUSY_CHECK
#endif

// 定义半双工SDA模式时，MISO引脚应设为-1（禁用）
#ifdef TFT_SDA_READ
  #ifdef TFT_MISO
    #if TFT_MISO != -1
      #undef TFT_MISO
      #define TFT_MISO -1
      #warning TFT_MISO set to -1  // 警告：MISO已设为-1
    #endif
  #endif
#endif  

/***************************************************************************************
**                         Section 4: Setup fonts
**                         第4节：字体配置
***************************************************************************************/
// 当用户请求不存在的平滑字体时，使用GLCD字体（临时修复ESP32重启问题）
#ifdef SMOOTH_FONT
  #ifndef LOAD_GLCD
    #define LOAD_GLCD
  #endif
#endif

// 仅加载User_Setup.h中定义的字体（节省空间）
// 设置标志位，可选编译RLE渲染代码
#ifdef LOAD_GLCD  // 加载GLCD字体（默认8x8字体）
  #include <Fonts/glcdfont.c>
#endif

#ifdef LOAD_FONT2  // 加载16号字体
  #include <Fonts/Font16.h>
#endif

#ifdef LOAD_FONT4  // 加载32号RLE压缩字体
  #include <Fonts/Font32rle.h>
  #define LOAD_RLE  // 标记启用RLE
#endif

#ifdef LOAD_FONT6  // 加载64号RLE压缩字体
  #include <Fonts/Font64rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT7  // 加载7段式RLE压缩字体
  #include <Fonts/Font7srle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_FONT8  // 加载72号RLE压缩字体
  #include <Fonts/Font72rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#elif defined LOAD_FONT8N  // 可选更窄的72号字体
  #define LOAD_FONT8
  #include <Fonts/Font72x53rle.h>
  #ifndef LOAD_RLE
    #define LOAD_RLE
  #endif
#endif

#ifdef LOAD_GFXFF  // 加载GFX免费字体
  #include <Fonts/GFXFF/gfxfont.h>
  // 加载用户自定义字体
  #include <User_Setups/User_Custom_Fonts.h>
#endif // #ifdef LOAD_GFXFF

// 创建空默认字体（防止未使用字体时崩溃）
const  uint8_t widtbl_null[1] = {0};          // 空宽度表
PROGMEM const uint8_t chr_null[1] = {0};      // 空字符表
PROGMEM const uint8_t* const chrtbl_null[1] = {chr_null};  // 空字符地址表

// 定义存储默认字体信息的结构体
// 存储：字体字符图像地址表指针、宽度表指针、高度、基线
typedef struct {
    const uint8_t *chartbl;    // 字符图像表指针
    const uint8_t *widthtbl;   // 字符宽度表指针
    uint8_t height;            // 字体高度
    uint8_t baseline;          // 字体基线（字符底部对齐线）
    } fontinfo;

// 填充字体信息结构体
const PROGMEM fontinfo fontdata [] = {
  #ifdef LOAD_GLCD
   { (const uint8_t *)font, widtbl_null, 0, 0 },  // GLCD字体（无完整参数）
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },  // 空字体
  #endif
   // GLCD字体（字体1）无完整参数，补充默认值
   { (const uint8_t *)chrtbl_null, widtbl_null, 8, 7 },

  #ifdef LOAD_FONT2
   { (const uint8_t *)chrtbl_f16, widtbl_f16, chr_hgt_f16, baseline_f16},  // 16号字体
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

   // 字体3暂未使用
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT4
   { (const uint8_t *)chrtbl_f32, widtbl_f32, chr_hgt_f32, baseline_f32},  // 32号字体
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

   // 字体5暂未使用
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },

  #ifdef LOAD_FONT6
   { (const uint8_t *)chrtbl_f64, widtbl_f64, chr_hgt_f64, baseline_f64},  // 64号字体
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT7
   { (const uint8_t *)chrtbl_f7s, widtbl_f7s, chr_hgt_f7s, baseline_f7s},  // 7段式字体
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 },
  #endif

  #ifdef LOAD_FONT8
   { (const uint8_t *)chrtbl_f72, widtbl_f72, chr_hgt_f72, baseline_f72}  // 72号字体
  #else
   { (const uint8_t *)chrtbl_null, widtbl_null, 0, 0 }
  #endif
};

/***************************************************************************************
**                         Section 5: Font datum enumeration
**                         第5节：文本基准点枚举
***************************************************************************************/
// 文本绘制对齐方式（参考基准点）
#define TL_DATUM 0 // 左上（默认）
#define TC_DATUM 1 // 上中
#define TR_DATUM 2 // 右上
#define ML_DATUM 3 // 中左（同CL_DATUM）
#define CL_DATUM 3 // 中左
#define MC_DATUM 4 // 中中（同CC_DATUM）
#define CC_DATUM 4 // 中中
#define MR_DATUM 5 // 中右（同CR_DATUM）
#define CR_DATUM 5 // 中右
#define BL_DATUM 6 // 左下
#define BC_DATUM 7 // 下中
#define BR_DATUM 8 // 右下
#define L_BASELINE  9 // 左基线（字符'A'的基线）
#define C_BASELINE 10 // 中基线
#define R_BASELINE 11 // 右基线

/***************************************************************************************
**                         Section 6: Colour enumeration
**                         第6节：颜色枚举
***************************************************************************************/
// 默认颜色定义（16位RGB565格式）
#define TFT_BLACK       0x0000      /*   0,   0,   0 */
#define TFT_NAVY        0x000F      /*   0,   0, 128 */
#define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define TFT_MAROON      0x7800      /* 128,   0,   0 */
#define TFT_PURPLE      0x780F      /* 128,   0, 128 */
#define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
#define TFT_LIGHTGREY   0xD69A      /* 211, 211, 211 */
#define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
#define TFT_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_CYAN        0x07FF      /*   0, 255, 255 */
#define TFT_RED         0xF800      /* 255,   0,   0 */
#define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
#define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
#define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
#define TFT_ORANGE      0xFDA0      /* 255, 180,   0 */
#define TFT_GREENYELLOW 0xB7E0      /* 180, 255,   0 */
#define TFT_PINK        0xFE19      /* 255, 192, 203 */ // 浅粉色（原0xFC9F）
#define TFT_BROWN       0x9A60      /* 150,  75,   0 */
#define TFT_GOLD        0xFEA0      /* 255, 215,   0 */
#define TFT_SILVER      0xC618      /* 192, 192, 192 */
#define TFT_SKYBLUE     0x867D      /* 135, 206, 235 */
#define TFT_VIOLET      0x915C      /* 180,  46, 226 */

// 特殊16位颜色值：编码为8位后能解码回原值
// 适用于8位/16位透明精灵图
#define TFT_TRANSPARENT 0x0120 // 实际是深绿色

// 4位颜色精灵图的默认调色板（PROGMEM存储）
static const uint16_t default_4bit_palette[] PROGMEM = {
  TFT_BLACK,    //  0  ^
  TFT_BROWN,    //  1  |
  TFT_RED,      //  2  |
  TFT_ORANGE,   //  3  |
  TFT_YELLOW,   //  4  0-9色遵循电阻色码！
  TFT_GREEN,    //  5  |
  TFT_BLUE,     //  6  |
  TFT_PURPLE,   //  7  |
  TFT_DARKGREY, //  8  |
  TFT_WHITE,    //  9  v
  TFT_CYAN,     // 10  蓝+绿
  TFT_MAGENTA,  // 11  蓝+红
  TFT_MAROON,   // 12  深红
  TFT_DARKGREEN,// 13  深绿
  TFT_NAVY,     // 14  深蓝
  TFT_PINK      // 15
};

/***************************************************************************************
**                         Section 7: Diagnostic support
**                         第7节：诊断支持
***************************************************************************************/
// #define TFT_eSPI_DEBUG     // 启用调试串口输出（暂未使用）
// #define TFT_eSPI_FNx_DEBUG // 启用函数x的调试（暂未使用）

// 运行时获取用户配置参数的结构体（调用getSetup()）
// 仅在使用时占用空间，主要用于诊断
typedef struct
{
String  version = TFT_ESPI_VERSION;  // 库版本
String  setup_info;                  // 用户配置名称
uint32_t setup_id;                   // 用户配置ID
int32_t esp;                         // 处理器代码
uint8_t trans;                       // SPI事务支持
uint8_t serial;                      // 串行(SPI)或并行接口
#ifndef GENERIC_PROCESSOR
uint8_t  port;                       // SPI端口
#endif
uint8_t overlap;                     // ESP8266重叠模式
uint8_t interface;                   // 接口类型

uint16_t tft_driver;                 // 驱动代码（十六进制）
uint16_t tft_width;                  // 旋转0时的宽度
uint16_t tft_height;                 // 旋转0时的高度

uint8_t r0_x_offset;                 // 各旋转角度的显示偏移
uint8_t r0_y_offset;
uint8_t r1_x_offset;
uint8_t r1_y_offset;
uint8_t r2_x_offset;
uint8_t r2_y_offset;
uint8_t r3_x_offset;
uint8_t r3_y_offset;

int8_t pin_tft_mosi; // SPI引脚
int8_t pin_tft_miso;
int8_t pin_tft_clk;
int8_t pin_tft_cs;

int8_t pin_tft_dc;   // 控制引脚
int8_t pin_tft_rd;
int8_t pin_tft_wr;
int8_t pin_tft_rst;

int8_t pin_tft_d0;   // 并行端口引脚
int8_t pin_tft_d1;
int8_t pin_tft_d2;
int8_t pin_tft_d3;
int8_t pin_tft_d4;
int8_t pin_tft_d5;
int8_t pin_tft_d6;
int8_t pin_tft_d7;

int8_t pin_tft_led;  // 背光引脚
int8_t pin_tft_led_on; // 背光开启电平

int8_t pin_tch_cs;   // 触摸芯片CS引脚

int16_t tft_spi_freq;// TFT写SPI频率
int16_t tft_rd_freq; // TFT读SPI频率
int16_t tch_spi_freq;// 触摸SPI频率
} setup_t;

/***************************************************************************************
**                         Section 8: Class member and support functions
**                         第8节：类成员和支持函数
***************************************************************************************/

// 平滑字体像素颜色读取的回调函数原型
typedef uint16_t (*getColorCallback)(uint16_t x, uint16_t y);

// TFT_eSPI类定义（继承Print类，TFT_eSprite友元类可访问保护成员）
class TFT_eSPI : public Print { friend class TFT_eSprite;

 //--------------------------------------- public ------------------------------------//
 public:

  // 构造函数（默认宽高为TFT_WIDTH/TFT_HEIGHT）
  TFT_eSPI(int16_t _W = TFT_WIDTH, int16_t _H = TFT_HEIGHT);

  // init()和begin()等效，begin()为向后兼容
  // tabcolor仅用于ST7735屏
  void     init(uint8_t tc = TAB_COLOUR), begin(uint8_t tc = TAB_COLOUR);

  // 虚函数（允许TFT_eSprite重写为精灵图专用函数）
  // 绘制像素
  virtual void     drawPixel(int32_t x, int32_t y, uint32_t color),
                   // 绘制字符
                   drawChar(int32_t x, int32_t y, uint16_t c, uint32_t color, uint32_t bg, uint8_t size),
                   // 绘制直线
                   drawLine(int32_t xs, int32_t ys, int32_t xe, int32_t ye, uint32_t color),
                   // 快速绘制垂直线
                   drawFastVLine(int32_t x, int32_t y, int32_t h, uint32_t color),
                   // 快速绘制水平线
                   drawFastHLine(int32_t x, int32_t y, int32_t w, uint32_t color),
                   // 填充矩形
                   fillRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);

  // 虚函数：绘制字符（返回字符宽度）
  virtual int16_t  drawChar(uint16_t uniCode, int32_t x, int32_t y, uint8_t font),
                   drawChar(uint16_t uniCode, int32_t x, int32_t y),
                   // 获取屏幕高度
                   height(void),
                   // 获取屏幕宽度
                   width(void);

                   // 读取x,y处像素颜色（RGB565格式）
  virtual uint16_t readPixel(int32_t x, int32_t y);

  // 设置窗口（参数：起始+结束坐标）
  virtual void     setWindow(int32_t xs, int32_t ys, int32_t xe, int32_t ye);  

  // 向设置的窗口写入像素颜色
  virtual void     pushColor(uint16_t color);

  // 非内联函数（允许重写）
  virtual void     begin_nin_write();  // 开始写入
  virtual void     end_nin_write();    // 结束写入

  // 设置显示旋转（0-3）
  void     setRotation(uint8_t r);
  // 获取当前旋转角度
  uint8_t  getRotation(void);

  // 更改原点位置（默认左上）
  // 注意：setRotation/setViewport/resetViewport会重置原点
  void     setOrigin(int32_t x, int32_t y);
  // 获取原点X坐标
  int32_t  getOriginX(void);
  // 获取原点Y坐标
  int32_t  getOriginY(void);

  // 反转显示颜色
  void     invertDisplay(bool i);

  // 设置地址窗口（参数：起始坐标+宽高）
  void     setAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h);

  // 视口命令（参考Viewport_Demo示例）
  // 设置视口（x,y:起始坐标；w,h:宽高；vpDatum:基准点是否受视口影响）
  void     setViewport(int32_t x, int32_t y, int32_t w, int32_t h, bool vpDatum = true);
  // 检查区域是否在视口内
  bool     checkViewport(int32_t x, int32_t y, int32_t w, int32_t h);
  // 获取视口X坐标
  int32_t  getViewportX(void);
  // 获取视口Y坐标
  int32_t  getViewportY(void);
  // 获取视口宽度
  int32_t  getViewportWidth(void);
  // 获取视口高度
  int32_t  getViewportHeight(void);
  // 获取视口基准点标志
  bool     getViewportDatum(void);
  // 绘制视口边框
  void     frameViewport(uint16_t color, int32_t w);
  // 重置视口
  void     resetViewport(void);

           // 裁剪地址窗口到视口边界，完全超出返回false
  bool     clipAddrWindow(int32_t* x, int32_t* y, int32_t* w, int32_t* h);
           // 裁剪窗口区域到视口边界，完全超出返回false
  bool     clipWindow(int32_t* xs, int32_t* ys, int32_t* xe, int32_t* ye);

           // 写入多个相同颜色（已废弃，用pushBlock）
  void     pushColor(uint16_t color, uint32_t len),
           // 写入颜色数组（带字节交换选项）
           pushColors(uint16_t  *data, uint32_t len, bool swap = true),
           // 写入8位颜色数组（已废弃，用pushPixels）
           pushColors(uint8_t  *data, uint32_t len);

           // 写入纯色块
  void     pushBlock(uint16_t color, uint32_t len);

           // 写入内存中的像素（用setSwapBytes调整字节序）
  void     pushPixels(const void * data_in, uint32_t len);

           // 半双工SDA SPI总线支持（MOSI需切换为输入）
           #ifdef TFT_SDA_READ
             #if defined (TFT_eSPI_ENABLE_8_BIT_READ)
  // 从TFT命令寄存器读取8位值
  uint8_t  tft_Read_8(void);
             #endif
  // 开始半双工SPI读取（MOSI设为输入）
  void     begin_SDA_Read(void);
  // 恢复MOSI为输出
  void     end_SDA_Read(void);
           #endif

  // 图形绘制
  // 填充屏幕
  void     fillScreen(uint32_t color),
           // 绘制矩形边框
           drawRect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color),
           // 绘制圆角矩形边框
           drawRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color),
           // 填充圆角矩形
           fillRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color);

  // 垂直渐变填充矩形
  void     fillRectVGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);
  // 水平渐变填充矩形
  void     fillRectHGradient(int16_t x, int16_t y, int16_t w, int16_t h, uint32_t color1, uint32_t color2);

  // 绘制圆形边框
  void     drawCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           // 圆形绘制辅助函数
           drawCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, uint32_t color),
           // 填充圆形
           fillCircle(int32_t x, int32_t y, int32_t r, uint32_t color),
           // 圆形填充辅助函数
           fillCircleHelper(int32_t x, int32_t y, int32_t r, uint8_t cornername, int32_t delta, uint32_t color),

           // 绘制椭圆边框
           drawEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),
           // 填充椭圆
           fillEllipse(int16_t x, int16_t y, int32_t rx, int32_t ry, uint16_t color),

           // 绘制三角形边框
           drawTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color),
           // 填充三角形
           fillTriangle(int32_t x1,int32_t y1, int32_t x2,int32_t y2, int32_t x3,int32_t y3, uint32_t color);

  // 平滑（抗锯齿）图形绘制
           // 绘制带透明度的像素（与背景色混合），返回混合后的颜色
           // 未指定bg_color时，从TFT/精灵图读取背景色
  uint16_t drawPixel(int32_t x, int32_t y, uint32_t color, uint8_t alpha, uint32_t bg_color = 0x00FFFFFF);

           // 绘制抗锯齿圆弧（起始/结束角度），可选圆角端点
           // 角度0=6点钟方向，90=9点钟方向，范围0-360（自动裁剪）
           // 起始角度可大于结束角度，圆弧始终顺时针绘制
  void     drawSmoothArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool roundEnds = false);

           // 绘制圆弧（端点不抗锯齿，便于动态分段）
           // 侧边默认抗锯齿，smoothArc=false时关闭
  void     drawArc(int32_t x, int32_t y, int32_t r, int32_t ir, uint32_t startAngle, uint32_t endAngle, uint32_t fg_color, uint32_t bg_color, bool smoothArc = true);

           // 绘制抗锯齿圆形边框
           // 线宽3像素（减少窄线抗锯齿的"编织"效应）
  void     drawSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t fg_color, uint32_t bg_color);
  
           // 填充抗锯齿圆形
           // 未指定bg_color时，从TFT/精灵图读取背景色
  void     fillSmoothCircle(int32_t x, int32_t y, int32_t r, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

           // 绘制抗锯齿圆角矩形边框（线宽r-ir+1）
           // 内外边框均抗锯齿，quadrants指定绘制的象限
  void     drawSmoothRoundRect(int32_t x, int32_t y, int32_t r, int32_t ir, int32_t w, int32_t h, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF, uint8_t quadrants = 0xF);

           // 填充抗锯齿圆角矩形
  void     fillSmoothRoundRect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t radius, uint32_t color, uint32_t bg_color = 0x00FFFFFF);

           // 绘制小抗锯齿填充圆点（使用drawWideLine）
  void     drawSpot(float ax, float ay, float r, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

           // 绘制抗锯齿宽线（两端圆角，半径wd/2）
  void     drawWideLine(float ax, float ay, float bx, float by, float wd, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

           // 绘制两端宽度不同的抗锯齿宽线（两端圆角）
  void     drawWedgeLine(float ax, float ay, float bx, float by, float aw, float bw, uint32_t fg_color, uint32_t bg_color = 0x00FFFFFF);

  // 图像渲染
           // 设置pushImage/pushPixels的字节交换（修正字节序）
  void     setSwapBytes(bool swap);
           // 获取字节交换状态
  bool     getSwapBytes(void);

           // 绘制位图
  void     drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawBitmap( int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           // 绘制X位图（位反转）
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor),
           drawXBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint16_t fgcolor, uint16_t bgcolor),
           // 设置1bpp精灵图的两种颜色
           setBitmapColor(uint16_t fgcolor, uint16_t bgcolor);

           // 设置TFT旋转精灵图的支点
  void     setPivot(int16_t x, int16_t y);
           // 获取支点X坐标
  int16_t  getPivotX(void),
           // 获取支点Y坐标
           getPivotY(void);

           // 屏幕区域复制函数对
           // 读取像素块到缓冲区（缓冲区至少w*h个16位元素）
  void     readRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
           // 将readRect读取的像素块写入屏幕
  void     pushRect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);

           // 渲染RAM中的16bpp图像/精灵图（Sprite类用）
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data);
           // 渲染带透明色的16bpp图像
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *data, uint16_t transparent);

           // 渲染FLASH中的16bpp图像
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data, uint16_t transparent);
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint16_t *data);

           // 渲染1/4/8bpp图像（Sprite类pushSprite用）
           // bpp8=true:8bpp；cmap:4bpp调色板指针
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, bool bpp8 = true, uint16_t *cmap = nullptr);
           // 带透明色的8bpp图像
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t  *data, uint8_t  transparent, bool bpp8 = true, uint16_t *cmap = nullptr);
           // FLASH中的8bpp图像
  void     pushImage(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t *data, bool bpp8,  uint16_t *cmap = nullptr);

           // 渲染带1bpp掩码的16bpp图像
  void     pushMaskedImage(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t *img, uint8_t *mask);

           // 读取屏幕区域的RGB888颜色（用于屏幕截图）
           // w/h=1时读取单个像素，缓冲区至少w*h*3字节
  void     readRectRGB(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t *data);

  // 文本渲染（返回渲染文本的像素宽度）
  // 绘制整数（指定字体）
  int16_t  drawNumber(long intNumber, int32_t x, int32_t y, uint8_t font),
           // 绘制整数（当前字体）
           drawNumber(long intNumber, int32_t x, int32_t y),
           // 绘制浮点数（指定小数位数和字体）
           drawFloat(float floatNumber, uint8_t decimal, int32_t x, int32_t y, uint8_t font),
           // 绘制浮点数（当前字体）
           drawFloat(float floatNumber, uint8_t decimal, int32_t x, int32_t y),
           // 绘制字符串（指定字体）
           drawString(const char *string, int32_t x, int32_t y, uint8_t font),
           // 绘制字符串（当前字体）
           drawString(const char *string, int32_t x, int32_t y),
           // 绘制String类型字符串（指定字体）
           drawString(const String& string, int32_t x, int32_t y, uint8_t font),
           // 绘制String类型字符串（当前字体）
           drawString(const String& string, int32_t x, int32_t y),
           // 绘制居中字符串（已废弃，用setTextDatum+drawString）
           drawCentreString(const char *string, int32_t x, int32_t y, uint8_t font),
           // 绘制右对齐字符串（已废弃）
           drawRightString(const char *string, int32_t x, int32_t y, uint8_t font),
           drawCentreString(const String& string, int32_t x, int32_t y, uint8_t font),
           drawRightString(const String& string, int32_t x, int32_t y, uint8_t font);

  // 文本渲染和字体处理支持函数
  // 设置tft.print()的光标位置
  void     setCursor(int16_t x, int16_t y),
           // 设置光标位置和字体
           setCursor(int16_t x, int16_t y, uint8_t font);

  // 获取当前光标X坐标（随tft.print()移动）
  int16_t  getCursorX(void),
           // 获取当前光标Y坐标
           getCursorY(void);

  // 设置文本颜色（仅前景色，背景不覆盖）
  void     setTextColor(uint16_t color),
           // 设置文本前景/背景色（smooth字体可选背景填充）
           setTextColor(uint16_t fgcolor, uint16_t bgcolor, bool bgfill = false),
           // 设置字符大小倍数（放大像素）
           setTextSize(uint8_t size);

  // 设置文本自动换行（X/Y轴）
  void     setTextWrap(bool wrapX, bool wrapY = false);

  // 设置文本基准点（默认左上）
  void     setTextDatum(uint8_t datum);
  // 获取当前文本基准点
  uint8_t  getTextDatum(void);

  // 设置文本填充宽度（背景擦除）
  void     setTextPadding(uint16_t x_width);
  // 获取文本填充宽度
  uint16_t getTextPadding(void);

#ifdef LOAD_GFXFF
  // 设置GFX免费字体（NULL恢复默认）
  void     setFreeFont(const GFXfont *f = NULL),
           // 设置后续使用的字体编号
           setTextFont(uint8_t font);
#else
  // 兼容定义（防止报错）
  void     setFreeFont(uint8_t font),
           setTextFont(uint8_t font);
#endif

  // 获取字符串在指定字体下的宽度
  int16_t  textWidth(const char *string, uint8_t font),
           // 获取字符串在当前字体下的宽度
           textWidth(const char *string),
           // String类型字符串宽度
           textWidth(const String& string, uint8_t font),
           textWidth(const String& string),
           // 获取指定字体的高度
           fontHeight(int16_t font),
           // 获取当前字体的高度
           fontHeight(void);

           // 从UTF8字符串提取Unicode编码（库内部用）
  uint16_t decodeUTF8(uint8_t *buf, uint16_t *index, uint16_t remaining),
           decodeUTF8(uint8_t c);

           // 支持UTF8解码并绘制print流中的字符
  size_t   write(uint8_t);

           // 设置smooth字体抗锯齿的颜色回调函数
  void     setCallback(getColorCallback getCol);

  // 返回已加载字体的位掩码（仅调试用）
  uint16_t fontsLoaded(void);

  // 底层读写函数
  // SPI写字节（仅兼容）
  void     spiwrite(uint8_t);
#ifdef RM68120_DRIVER
  // 发送16位命令（重置DC/RS为高，准备数据）
  void     writecommand(uint16_t c);
  // 向16位命令寄存器写入8位数据
  void     writeRegister8(uint16_t c, uint8_t d);
  // 向16位命令寄存器写入16位数据
  void     writeRegister16(uint16_t c, uint16_t d);
#else
  // 发送8位命令
  void     writecommand(uint8_t c);
#endif
  // 发送数据（DC/RS设为高）
  void     writedata(uint8_t d);

  // 发送FLASH中的TFT初始化序列
  void     commandList(const uint8_t *addr);

  // 从TFT读取8/16/32位数据
  uint8_t  readcommand8( uint8_t cmd_function, uint8_t index = 0);
  uint16_t readcommand16(uint8_t cmd_function, uint8_t index = 0);
  uint32_t readcommand32(uint8_t cmd_function, uint8_t index = 0);

  // 颜色转换
           // 8位RGB转16位RGB565
  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue);

           // 8位颜色转16位
  uint16_t color8to16(uint8_t color332);
           // 16位颜色转8位
  uint8_t  color16to8(uint16_t color565);

           // 16位转24位RGB（低24位）
  uint32_t color16to24(uint16_t color565);
           // 24位转16位
  uint32_t color24to16(uint32_t color888);

           // 颜色混合（alpha:0=全背景，255=全前景）
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc);

           // 带抖动的16位颜色混合（减少色带）
  uint16_t alphaBlend(uint8_t alpha, uint16_t fgc, uint16_t bgc, uint8_t dither);
           // 24位颜色混合（可选抖动）
  uint32_t alphaBlend24(uint8_t alpha, uint32_t fgc, uint32_t bgc, uint8_t dither = 0);

  // DMA支持函数（ESP32/STM32/RP2040）
  // 初始化DMA并绑定到SPI（setup中用）
  // ctrl_cs=true:DMA控制TFT CS（ESP32仅，此时TFT读失效）
  bool     initDMA(bool ctrl_cs = false);
  // 反初始化DMA
  void     deInitDMA(void);

           // DMA推送图像（可选双缓冲区）
           // 缓冲区用于防止图像数据在DMA时被修改
           // 注意：未用缓冲区时，原图像可能被字节交换/裁剪修改
           // 函数会等待上一次DMA完成
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* data, uint16_t* buffer = nullptr);

#if defined (ESP32) // ESP32专属
           // 常量图像指针的DMA推送（防止修改原数据）
  void     pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t const* data);
#endif
           // 向setAddrWindow设置的窗口DMA推送像素
  void     pushPixelsDMA(uint16_t* image, uint32_t len);

           // 检查DMA是否完成
  bool     dmaBusy(void);
  // 等待DMA完成
  void     dmaWait(void);

  bool     DMA_Enabled = false;   // DMA启用标志
  uint8_t  spiBusyCheck = 0;      // ESP32传输缓冲区检查数量

  // 底层函数
  // 开始SPI事务
  void     startWrite(void);
  // 写入多个相同颜色（已废弃）
  void     writeColor(uint16_t color, uint32_t len);
  // 结束SPI事务
  void     endWrite(void);

  // 设置/获取库配置属性
  // id=1: 启用/禁用GLCD cp437字体纠错
  // id=2: 启用/禁用UTF8解码
  // id=3: 启用/禁用ESP32 PSRAM
  #define CP437_SWITCH 1
  #define UTF8_SWITCH  2
  #define PSRAM_ENABLE 3
  void     setAttribute(uint8_t id = 0, uint8_t a = 0);
  uint8_t  getAttribute(uint8_t id = 0);

           // 获取库配置参数（诊断用）
  void     getSetup(setup_t& tft_settings);
  // 验证配置ID
  bool     verifySetupID(uint32_t id);

  // 全局变量
#if !defined (TFT_PARALLEL_8_BIT) && !defined (RP2040_PIO_INTERFACE)
  // 获取SPI类句柄
  static   SPIClass& getSPIinstance(void);
#endif
  uint32_t textcolor, textbgcolor; // 文本前景/背景色

  uint32_t bitmap_fg, bitmap_bg;   // 位图前景/背景色

  uint8_t  textfont,  // 当前字体编号
           textsize,  // 当前字体大小倍数
           textdatum, // 文本基准点
           rotation;  // 显示旋转角度

  uint8_t  decoderState = 0;   // UTF8解码器状态
  uint16_t decoderBuffer;      // Unicode编码缓冲区

 //--------------------------------------- private ------------------------------------//
 private:
           // 兼容函数（已废弃，待删除）
  void     spi_begin();
  void     spi_end();

  // SPI读开始/结束
  void     spi_begin_read();
  void     spi_end_read();

           // 新的读写开始/结束（内联）
  inline void begin_tft_write() __attribute__((always_inline));
  inline void end_tft_write()   __attribute__((always_inline));
  inline void begin_tft_read()  __attribute__((always_inline));
  inline void end_tft_read()    __attribute__((always_inline));

           // 初始化数据总线GPIO和硬件接口
  void     initBus(void);

           // 临时函数（开发用，待删除）
  void     pushSwapBytePixels(const void* data_in, uint32_t len);

           // 设置读地址窗口（退出时CGRAM为读模式）
  void     readAddrWindow(int32_t xs, int32_t ys, int32_t w, int32_t h);

           // 读取字节
  uint8_t  readByte(void);

           // 并行总线GPIO方向控制
  void     busDir(uint32_t mask, uint8_t mode);

           // 单个GPIO方向控制
  void     gpioMode(uint8_t gpio, uint8_t mode);

           // 平滑图形辅助函数
  uint8_t  sqrt_fraction(uint32_t num);

           // 计算点到线段的距离
  float    wedgeLineDistance(float pax, float pay, float bax, float bay, float dr);

           // 显示变体设置
  uint8_t  tabcolor,                   // ST7735标签色（无效）
           colstart = 0, rowstart = 0; // 屏幕到CGRAM的坐标偏移

           // ESP8266控制信号端口/引脚掩码
  volatile uint32_t *dcport, *csport;
  uint32_t cspinmask, dcpinmask, wrpinmask, sclkpinmask;

           #if defined(ESP32_PARALLEL)
           // ESP32并行总线掩码
  uint32_t xclr_mask, xdir_mask;
  // ESP32并行总线查找表（1KB RAM，提升精灵图渲染速度）
  uint32_t xset_mask[256];
           #endif

  getColorCallback getColor = nullptr; // 平滑字体颜色回调

  bool     locked, inTransaction, lockTransaction; // SPI事务/互斥锁标志

 //-------------------------------------- protected ----------------------------------//
 protected:

  int32_t  _init_width, _init_height; // 初始宽高（setRotation用）
  int32_t  _width, _height;           // 当前旋转的宽高
  int32_t  addr_row, addr_col;        // 窗口位置（减少窗口命令）

  int16_t  _xPivot;   // 旋转精灵图的X支点
  int16_t  _yPivot;   // 旋转精灵图的Y支点

  // 视口变量
  int32_t  _vpX, _vpY, _vpW, _vpH;    // 视口坐标/宽高
  int32_t  _xDatum;
  int32_t  _yDatum;
  int32_t  _xWidth;
  int32_t  _yHeight;
  bool     _vpDatum;
  bool     _vpOoB;

  int32_t  cursor_x, cursor_y, padX;       // 文本光标/填充
  int32_t  bg_cursor_x;                    // 背景填充光标
  int32_t  last_cursor_x;                  // 上一次光标位置

  uint32_t fontsloaded;               // 已加载字体的位掩码

  uint8_t  glyph_ab,   // 平滑字体基线以上高度
           glyph_bb;   // 平滑字体基线以下高度

  bool     isDigits;   // 数字边界框调整（减少抖动）
  bool     textwrapX, textwrapY;  // 文本换行标志
  bool     _swapBytes; // 字节交换标志

  bool     _booted;    // init/begin已执行标志

  bool     _cp437;        // CP437字体纠错
  bool     _utf8;         // UTF8解码
  bool     _psram_enable; // PSRAM启用

  uint32_t _lastColor; // 上一次颜色（减少位运算）

  bool     _fillbg;    // 背景填充标志（仅smooth字体）

#if defined (SSD1963_DRIVER)
  uint16_t Cswap;      // SSD1963交换缓冲区
  uint8_t r6, g6, b6;  // SSD1963 RGB缓冲区
#endif

#ifdef LOAD_GFXFF
  GFXfont  *gfxFont;   // GFX字体指针
#endif

}; // TFT_eSPI类结束

/***************************************************************************************
**                         Section 9: TFT_eSPI class conditional extensions
**                         第9节：TFT_eSPI类条件扩展
***************************************************************************************/
// 加载触摸功能
#ifdef TOUCH_CS
  #if defined (TFT_PARALLEL_8_BIT) || defined (RP2040_PIO_INTERFACE)
    #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
      #error >>>>------>> Touch functions not supported in 8/16-bit parallel mode or with RP2040 PIO.
    #endif
  #else
    #include "Extensions/Touch.h"        // 加载触摸库
  #endif
#else
    #if !defined(DISABLE_ALL_LIBRARY_WARNINGS)
      #warning >>>>------>> TOUCH_CS pin not defined, TFT_eSPI touch functions will not be available!
    #endif
#endif

// 加载平滑字体功能
#ifdef SMOOTH_FONT
  #include "Extensions/Smooth_font.h"  // 加载平滑字体库
#endif

/***************************************************************************************
**                         Section 10: Additional extension classes
**                         第10节：额外扩展类
***************************************************************************************/
// 加载按钮类
#include "Extensions/Button.h"

// 加载精灵图类
#include "Extensions/Sprite.h"

// 交换任意类型的两个值
template <typename T> static inline void
transpose(T& a, T& b) { T t = a; a = b; b = t; }

// 快速颜色混合函数
template <typename A, typename F, typename B> static inline uint16_t
fastBlend(A alpha, F fgc, B bgc)
{
  // 混合5位红/蓝通道
  uint32_t rxb = bgc & 0xF81F;
  rxb += ((fgc & 0xF81F) - rxb) * (alpha >> 2) >> 6;
  // 混合6位绿色通道
  uint32_t xgx = bgc & 0x07E0;
  xgx += ((fgc & 0x07E0) - xgx) * alpha >> 8;
  // 重组通道
  return (rxb & 0xF81F) | (xgx & 0x07E0);
}

#endif // _TFT_eSPIH_ 头文件保护结束