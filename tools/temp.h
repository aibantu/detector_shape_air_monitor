#ifndef TEMP_PNG_H
#define TEMP_PNG_H

#include <stdint.h>

// 48×48 RGB565数组（透明色=0x0000，适配硬件位序）
extern const uint16_t temp_png[];
extern const uint32_t temp_png_len;
#define temp_png_W 48
#define temp_png_H 48
#define temp_png_TRANSPARENT 0x0000

#endif // TEMP_PNG_H
