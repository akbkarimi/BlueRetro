/*
 * 1 bit framebuffer + drawing primitives for an SSD1306 OLED.
 * Pure C, no hardware access. Local addition, not part of upstream BlueRetro.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UI_GFX_H_
#define _UI_GFX_H_

#include <stdint.h>
#include "ui_config.h"

#define GFX_W        128
#define GFX_H        UI_OLED_HEIGHT
#define GFX_BUF_SIZE (GFX_W * GFX_H / 8)    /* SSD1306 page layout: bit0 = top pixel of the page */

void gfx_clear(void);
void gfx_set_offset(int dx, int dy);        /* Added to every pixel drawn (burn-in pixel shift) */
void gfx_pixel(int x, int y, int on);
void gfx_line(int x0, int y0, int x1, int y1, int on);
void gfx_rect(int x, int y, int w, int h, int on);          /* Outline */
void gfx_fill_rect(int x, int y, int w, int h, int on);
void gfx_circle(int cx, int cy, int r, int on);             /* Outline */
void gfx_fill_circle(int cx, int cy, int r, int on);
int  gfx_text(int x, int y, const char *s);                 /* 5x7 font, upper case only. Returns end x */
const uint8_t *gfx_buffer(void);

#endif /* _UI_GFX_H_ */
