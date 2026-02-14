// ###########################################################################
// ESP32S3 + ST7789 + 172x320 屏幕 专属配置（修改后）
// ###########################################################################

// 1. 选择显示屏控制器（必须正确！）
#define ST7789_DRIVER  // 启用 ST7789 驱动

// 2. SPI 引脚定义（无背光，TFT_BL 设为 -1）
// SCL GPOI18
// SDA GPIO17
// TFT_RST GPIO16
// DC  GPIO15
// CS  GPIO7
// BL  GPIO6   
#define TFT_MISO -1    // SPI MISO 引脚（仅读显存时需要）
#define TFT_MOSI 17    // SPI MOSI 引脚（核心数据引脚）
#define TFT_SCLK 18    // SPI SCK 时钟引脚
#define TFT_CS   7    // 片选引脚（低电平有效）
#define TFT_DC   15    // 数据/命令引脚（必须接）
#define TFT_RST  16    // 复位引脚（可选，不接则设为 -1）
#define TFT_BL   6    // 背光引脚：未连接（设为 -1，库会跳过背光初始化）

// 3. 显示屏核心参数（不变）
#define TFT_WIDTH  240   // 屏幕实际宽度
#define TFT_HEIGHT 320   // 屏幕实际高度
#define TFT_ROTATION 1   // 屏幕旋转（0-3 按需调整）

#define ST7789_GREENTAB_240x320  
#define TFT_RGB_ORDER TFT_BGR     
// #define TFT_RGB_ORDER TFT_RGB    


#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
#define LOAD_FONT2  // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
#define LOAD_FONT6  // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
#define LOAD_FONT8  // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

// #define TFT_INIT_DELAY 100

// 5. ESP32S3 性能优化配置（不变）
#define SPI_FREQUENCY  80000000
#define SPI_READ_FREQUENCY 20000000
#define USE_DMA
//#define DMA_CHANNEL    1
#define USE_HSPI_PORT

// 6. 可选功能配置（按需求修改后）
#define SUPPORT_TRANSPARENT_SPRITES  // 启用透明精灵图层（动画常用，保留）
// #define TFT_ENABLE_BL  // 注释/删除：无背光控制，不启用该功能
#define USER_SETUP_LOADED  // 标记配置已加载（避免冲突，保留）
// #define USER_SETUP_ID 71  // 自定义配置 ID（唯一标识，保留）