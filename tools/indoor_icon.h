#ifndef ICON_INDOOR_H
#define ICON_INDOOR_H

#include <stdint.h>

// 48×48 RGB565数组（透明色=0x0000，适配硬件位序）
extern const uint16_t icon_indoor[];
extern const uint32_t icon_indoor_len;
#define icon_indoor_W 48
#define icon_indoor_H 48
#define icon_indoor_TRANSPARENT 0x0000

#endif // ICON_INDOOR_H
