#ifndef __FONT_H
#define __FONT_H

#include "stdint.h"

/**
 * @brief FontDef structure, describe a font of TFT display.
 *
 */
typedef struct FontDef{
    const uint8_t width;	/**< Width of a char in pixel. */
    uint8_t height;			/**< Height of a char in pixel. */
    const uint16_t *data;	/**< Font datas. */
} FontDef;

//Font lib.
extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

//16-bit(RGB565) Image lib.
/*******************************************
 *             CAUTION:
 *   If the MCU onchip flash cannot
 *  store such huge image data,please
 *           do not use it.
 * These pics are for test purpose only.
 *******************************************/

/* 128x128 pixel RGB565 image */
extern const uint16_t saber[][128];

/* 240x240 pixel RGB565 image 
extern const uint16_t knky[][240];
extern const uint16_t tek[][240];
extern const uint16_t adi1[][240];
*/
#endif
