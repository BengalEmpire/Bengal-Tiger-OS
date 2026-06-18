/**
 * Bengal Tiger OS - 8x16 Bitmap Font Data
 *
 * Standard VGA 8x16 font for text rendering in graphics mode.
 * Contains ASCII characters 32-126 (printable characters).
 * Each character is 16 bytes (8x16 pixels, 1 bit per pixel).
 *
 * @file font.h
 * @author Bengal Tiger OS (BengalEmpire)
 * @version 0.4.0
 */

#ifndef FONT_H
#define FONT_H

#include "common.h"

/* Character dimensions */
#define FONT_WIDTH  8
#define FONT_HEIGHT 16
#define FONT_FIRST_CHAR 32
#define FONT_LAST_CHAR  126
#define FONT_NUM_CHARS  (FONT_LAST_CHAR - FONT_FIRST_CHAR + 1)

/** Font bitmap data: FONT_NUM_CHARS rows of 16 bytes each */
extern const uint8_t font_8x16[FONT_NUM_CHARS][FONT_HEIGHT];

#endif /* FONT_H */
