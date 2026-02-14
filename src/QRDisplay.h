// #ifndef QRCODE_DISPLAY_H
// #define QRCODE_DISPLAY_H

// #include <TFT_eSPI.h>

// #endif

// #include <qrcore.h>
// void displayQRCode(TFT_eSprite* sprite, const char* text) {
//     sprite->fillScreen(TFT_BLACK);
//     sprite->setTextColor(TFT_WHITE);
//     sprite->setTextFont(2);
//     sprite->drawString("请扫码配置WiFi", 50, 20);

//     QRCode qrcode;
//     uint8_t qrcodeData[qrcode_getBufferSize(3)];
//     qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, text);

//     int qrSize = 128;
//     int x0 = (sprite->width() - qrSize) / 2;
//     int y0 = 60;
//     for (int y = 0; y < qrcode.size; y++) {
//         for (int x = 0; x < qrcode.size; x++) {
//             if (qrcode_getModule(&qrcode, x, y)) {
//                 sprite->fillRect(x0 + x*2, y0 + y*2, 2, 2, TFT_WHITE);
//             }
//         }
//     }
//     sprite->drawString("AP: " AP_SSID, 50, y0 + qrSize + 20);
// }