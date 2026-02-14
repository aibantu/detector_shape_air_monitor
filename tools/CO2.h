#ifndef CO2.PNG_H
#define CO2.PNG_H

#include <stdint.h>

// 48×48 RGB565数组（透明色=0x0000，适配硬件位序）
extern const uint16_t CO2.png[];
extern const uint32_t CO2.png_len;
#define CO2.png_W 48
#define CO2.png_H 48
#define CO2.png_TRANSPARENT 0x0000

#endif // CO2.PNG_H
