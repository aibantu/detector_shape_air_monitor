#ifndef ICO_FD09D628_DATA_H
#define ICO_FD09D628_DATA_H

#include <stdint.h>

// 48x48 RGB565 (transparent = 0x0000, byte-swapped)
extern const uint16_t ico_fd09d628_data[];
extern const uint32_t ico_fd09d628_data_len;
#define ico_fd09d628_data_W 48
#define ico_fd09d628_data_H 48
#define ico_fd09d628_data_TRANSPARENT 0x0000

#endif // ICO_FD09D628_DATA_H
