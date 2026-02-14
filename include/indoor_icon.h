#ifndef ICON_INDOOR_H
#define ICON_INDOOR_H

#include <stdint.h>

#define icon_indoor_W 48
#define icon_indoor_H 48
#define icon_indoor_PIXELS (icon_indoor_W * icon_indoor_H)
#define icon_indoor_TRANSPARENT 0x0000

// 48×48 RGB565 数组（透明色=0x0000）
// 用定长声明强制编译期校验：定义端如果不是 2304 个像素会报错。
extern const uint16_t icon_indoor[icon_indoor_PIXELS];
extern const uint32_t icon_indoor_len;

#endif // ICON_INDOOR_H
