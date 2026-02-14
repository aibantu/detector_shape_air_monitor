#ifndef HUMI.PNG_H
#define HUMI.PNG_H

#include <stdint.h>

// 48×48 RGB565数组（透明色=0x0000，适配硬件位序）
extern const uint16_t humi.png[];
extern const uint32_t humi.png_len;
#define humi.png_W 48
#define humi.png_H 48
#define humi.png_TRANSPARENT 0x0000

#endif // HUMI.PNG_H
