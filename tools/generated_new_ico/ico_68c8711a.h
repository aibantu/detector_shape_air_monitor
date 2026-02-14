#ifndef ICO_68C8711A_DATA_H
#define ICO_68C8711A_DATA_H

#include <stdint.h>

// 48x48 RGB565 (transparent = 0x0000, byte-swapped)
extern const uint16_t ico_68c8711a_data[];
extern const uint32_t ico_68c8711a_data_len;
#define ico_68c8711a_data_W 48
#define ico_68c8711a_data_H 48
#define ico_68c8711a_data_TRANSPARENT 0x0000

#endif // ICO_68C8711A_DATA_H
