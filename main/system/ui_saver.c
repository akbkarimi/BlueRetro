/*
 * OLED screensavers (PlayStation symbols, stars, PS pad, Tetris demo).
 * Local addition, not part of upstream BlueRetro.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sdkconfig.h"

#include "sdkconfig.h"

#ifdef CONFIG_BLUERETRO_SYSTEM_PSX_PS2

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui_gfx.h"
#include "ui_saver.h"

/* ---------------- Random (xorshift32) ---------------- */

static uint32_t rng_state = 1;

void saver_seed(uint32_t seed) {
    rng_state = seed ? seed : 1;
}

static uint32_t rnd(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static int rnd_range(int n) {
    return (int)(rnd() % (uint32_t)n);
}

/* ---------------- PlayStation symbols ---------------- */

#define NSYM 4

static uint32_t symbols_frame;
static int symbol_order[NSYM] = { 0, 1, 2, 3 };

/*
 * Four 16x16 monochrome bitmap symbols.
 * Each uint16_t represents one row, MSB = leftmost pixel.
 * Order: Cross, Circle, Triangle, Square.
 */
static const uint16_t symbol_bitmap[NSYM][16] = {
    /* Cross */
    {
        0xC003, 0x6006, 0x300C, 0x1818,
        0x0C30, 0x0660, 0x03C0, 0x0180,
        0x0180, 0x03C0, 0x0660, 0x0C30,
        0x1818, 0x300C, 0x6006, 0xC003
    },
    /* Circle */
    {
        0x03C0, 0x0FF0, 0x1C38, 0x381C,
        0x700E, 0x6006, 0x6006, 0xC003,
        0xC003, 0x6006, 0x6006, 0x700E,
        0x381C, 0x1C38, 0x0FF0, 0x03C0
    },
    /* Triangle */
    {
        0x0000, 0x0000, 0x0000, 0x0180,
        0x0180, 0x03C0, 0x03C0, 0x0660,
        0x0660, 0x0C30, 0x1818, 0x1818,
        0x300C, 0x7FFE, 0x7FFE, 0x0000
    },
    /* Square */
    {
        0x7FFE, 0x4002, 0x4002, 0x4002,
        0x4002, 0x4002, 0x4002, 0x4002,
        0x4002, 0x4002, 0x4002, 0x4002,
        0x4002, 0x4002, 0x4002, 0x7FFE
    }
};

static void draw_symbol(int kind, int cx, int cy)
{
    if (kind < 0 || kind >= NSYM) {
        return;
    }

    for (int y = 0; y < 16; y++) {
        uint16_t row = symbol_bitmap[kind][y];

        for (int x = 0; x < 16; x++) {
            if (row & (uint16_t)(0x8000u >> x)) {
                gfx_pixel(cx - 8 + x, cy - 8 + y, 1);
            }
        }
    }
}

static void symbols_start(void)
{
    symbols_frame = 0;
    symbol_order[0] = 0;
    symbol_order[1] = 1;
    symbol_order[2] = 2;
    symbol_order[3] = 3;
}

static void symbols_step(void)
{
    /* Four fixed positions; symbols exchange positions every 240 frames. */
    static const int px[NSYM] = { 16, 48, 80, 112 };
    static const int py[NSYM] = { 16, 16, 16, 16 };

    symbols_frame++;

    if (symbols_frame % 240U == 0U) {
        int first = symbol_order[0];
        symbol_order[0] = symbol_order[1];
        symbol_order[1] = symbol_order[3];
        symbol_order[3] = symbol_order[2];
        symbol_order[2] = first;
    }

    /* Draw all four symbols continuously; no temporal fading or flicker. */
    for (int i = 0; i < NSYM; i++) {
        draw_symbol(symbol_order[i], px[i], py[i]);
    }
}

static void draw_thick_line(int x0, int y0, int x1, int y1)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    if (abs(dx) >= abs(dy)) {
        gfx_line(x0, y0 - 1, x1, y1 - 1, 1);
        gfx_line(x0, y0,     x1, y1,     1);
        gfx_line(x0, y0 + 1, x1, y1 + 1, 1);
    } else {
        gfx_line(x0 - 1, y0, x1 - 1, y1, 1);
        gfx_line(x0,     y0, x1,     y1, 1);
        gfx_line(x0 + 1, y0, x1 + 1, y1, 1);
    }
}

/* ---------------- Galaxy starfield ---------------- */

#define NSTARS 42

typedef struct {
    int x, y;
    int dx, dy;
    int phase;
    int brightness;
    int cross;
} star_t;

static star_t stars[NSTARS];
static uint32_t stars_frame;

static void stars_start(void)
{
    stars_frame = 0;

    for (int i = 0; i < NSTARS; i++) {
        stars[i].x = rnd_range(GFX_W);
        stars[i].y = rnd_range(GFX_H);

        /* Most stars drift gently; some stay nearly still. */
        stars[i].dx = rnd_range(3) - 1;
        stars[i].dy = rnd_range(3) - 1;
        if (i % 4 == 0) {
            stars[i].dx = 0;
            stars[i].dy = 0;
        }

        stars[i].phase = rnd_range(8);
        stars[i].brightness = 2 + rnd_range(7);
        stars[i].cross = (i % 7 == 0);
    }
}

static void stars_step(void)
{
    stars_frame++;

    for (int i = 0; i < NSTARS; i++) {
        star_t *s = &stars[i];

        /* Move at different angles rather than all travelling left. */
        if ((stars_frame + (uint32_t)i) % 2U == 0U) {
            s->x += s->dx;
            s->y += s->dy;
        }

        if (s->x < 0) s->x = GFX_W - 1;
        if (s->x >= GFX_W) s->x = 0;
        if (s->y < 0) s->y = GFX_H - 1;
        if (s->y >= GFX_H) s->y = 0;

        /* Different phase and duty cycle gives each star its own twinkle. */
        int twinkle = (int)((stars_frame + (uint32_t)s->phase) % 12U);
        if (twinkle < s->brightness) {
            gfx_pixel(s->x, s->y, 1);
        }

        /* Bright stars briefly flare into a small four-point cross. */
        if (s->cross && twinkle >= 2 && twinkle <= 5) {
            gfx_pixel(s->x - 1, s->y, 1);
            gfx_pixel(s->x + 1, s->y, 1);
            gfx_pixel(s->x, s->y - 1, 1);
            gfx_pixel(s->x, s->y + 1, 1);
        }
    }
}

/* ---------------- Console logo carousel ---------------- */

static uint32_t logo_frame;


/* Image2CPP 128x32 monochrome bitmaps: 512 bytes each. */
static const uint8_t logo_bitmap_images23[512] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xfc, 0x00, 0x1f, 0xe3, 0x80, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0xfe, 0x00, 0x1f, 0xf3, 0x80, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0xf7, 0x00, 0x1c, 0x33, 0x80, 0x00, 0x00, 0xe0, 0x38, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xf7, 0x80, 0x1c, 0x3b, 0x83, 0xe6, 0x06, 0xe0, 0x3f, 0x0f, 0x8f, 0x98, 0x30, 0x3e, 0x00,
    0x00, 0xf7, 0x80, 0x1c, 0x3b, 0x8f, 0xf7, 0x0e, 0xe0, 0x3f, 0x3f, 0xcf, 0x98, 0xfe, 0x3f, 0x80,
    0x00, 0xf7, 0x80, 0x1c, 0x3b, 0x9c, 0x73, 0x0c, 0xe0, 0x38, 0x71, 0xcc, 0x19, 0xc6, 0x31, 0xc0,
    0x00, 0xf7, 0x00, 0x1c, 0x33, 0x98, 0x73, 0x8c, 0x7e, 0x38, 0x61, 0xcc, 0x19, 0xc7, 0x30, 0xc0,
    0x00, 0xf7, 0x00, 0x1d, 0xf3, 0x98, 0x73, 0x9c, 0x3f, 0x38, 0x61, 0xcc, 0x19, 0x87, 0x30, 0xe0,
    0x03, 0xf0, 0x00, 0x1d, 0xe3, 0x98, 0x71, 0x98, 0x07, 0x38, 0x61, 0xcc, 0x19, 0x83, 0x30, 0xe0,
    0x1f, 0xff, 0xf0, 0x1c, 0x03, 0x98, 0x71, 0xf8, 0x03, 0xb8, 0x61, 0xcc, 0x19, 0x87, 0x30, 0xe0,
    0x78, 0xfc, 0x38, 0x1c, 0x03, 0x98, 0x71, 0xf0, 0x03, 0xb8, 0x61, 0xcc, 0x19, 0x87, 0x30, 0xe0,
    0x73, 0xf1, 0xf0, 0x1c, 0x03, 0x98, 0x70, 0xf0, 0x03, 0x38, 0x61, 0xcc, 0x19, 0xc7, 0x30, 0xe0,
    0x1f, 0xff, 0xc0, 0x1c, 0x03, 0x98, 0x70, 0xf0, 0x07, 0x18, 0x71, 0xce, 0x19, 0xc6, 0x30, 0xe0,
    0x00, 0xfe, 0x00, 0x1c, 0x03, 0x9f, 0x70, 0xe0, 0x7e, 0x1e, 0x3d, 0xc7, 0x18, 0xfc, 0x30, 0xe0,
    0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t logo_bitmap_images2[512] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x07, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x07, 0xe0, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0xf8, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x07, 0x9c, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x07, 0x9e, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x07, 0x9e, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x9e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x9e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x9e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x9e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x07, 0x9c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x37, 0x80, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xf7, 0x9f, 0xe3, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0xf7, 0xbe, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4f, 0x87, 0xb0, 0xfa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x37, 0x87, 0xf6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x67, 0xf7, 0x9f, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x07, 0xbc, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x03, 0xb0, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x01, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0, 0x07, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};


static void draw_bitmap_128x32(const uint8_t *bitmap)
{
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 128; x++) {
            uint8_t byte = bitmap[y * 16 + (x >> 3)];
            if (byte & (uint8_t)(0x80u >> (x & 7))) {
                gfx_pixel(x, y, 1);
            }
        }
    }
}

static void draw_ps_logo(void)
{
    draw_bitmap_128x32(logo_bitmap_images2);
}

static void draw_ps_banner(void)
{
    draw_bitmap_128x32(logo_bitmap_images23);
}

static void draw_xbox_logo(void)
{
    gfx_circle(64, 13, 11, 1);
    draw_thick_line(57, 6, 71, 20);
    draw_thick_line(71, 6, 57, 20);
    gfx_text(49, 25, "XBOX");
}

static void draw_nintendo_logo(void)
{
    gfx_rect(39, 4, 50, 19, 1);
    gfx_rect(41, 6, 46, 15, 1);
    gfx_text(45, 10, "NINTENDO");
}

static void draw_sega_logo(void)
{
    gfx_text(43, 8, "SEGA");
    gfx_line(42, 17, 84, 17, 1);
    gfx_line(44, 20, 82, 20, 1);
}

static void pad_start(void)
{
    logo_frame = 0;
}

static void pad_step(void)
{
    logo_frame++;

    /* Change logo every 180 frames; brief blank intervals imitate a fade. */
    uint32_t phase = logo_frame % 180U;
    if (phase >= 8U && phase < 172U) {
        switch ((logo_frame / 180U) % 5U) {
            case 0: draw_ps_logo(); break;
            case 1: draw_ps_banner(); break;
            case 2: draw_xbox_logo(); break;
            case 3: draw_nintendo_logo(); break;
            default: draw_sega_logo(); break;
        }
    }
}

/* ---------------- Tetris (plays itself) ----------------
 *
 * The 10 x 20 field is drawn rotated: columns run down the screen (10 x 3 px = 30 px,
 * fits the 32 px high display) and pieces fall from left to right (20 x 3 px = 60 px).
 */

#define TW   42     /* 42 columns x 3 pixels = 126 pixels */
#define TH   10     /* 10 rows x 3 pixels = 30 pixels */
#define CELL_W 3
#define CELL_H 3
#define TX0 ((GFX_W - TW * CELL_W) / 2)
#define TY0 ((GFX_H - TH * CELL_H) / 2)
#define OVER_FRAMES 25

/* Cells (x, y) inside a 4x4 box */
static const int8_t shapes[7][4][2] = {
    { {0,1}, {1,1}, {2,1}, {3,1} },     /* I */
    { {1,1}, {2,1}, {1,2}, {2,2} },     /* O */
    { {0,1}, {1,1}, {2,1}, {1,2} },     /* T */
    { {1,1}, {2,1}, {0,2}, {1,2} },     /* S */
    { {0,1}, {1,1}, {1,2}, {2,2} },     /* Z */
    { {0,1}, {1,1}, {2,1}, {0,2} },     /* J */
    { {0,1}, {1,1}, {2,1}, {2,2} },     /* L */
};

typedef struct {
    int shape, rot, bx, by;
    int t_rot, t_bx;
} piece_t;

static uint8_t board[TH][TW];
static piece_t cur;
static uint32_t t_score, t_lines, t_games;
static int t_over;

static void piece_cells(int shape, int rot, int8_t out[4][2]) {
    for (int i = 0; i < 4; i++) {
        int x = shapes[shape][i][0];
        int y = shapes[shape][i][1];

        for (int r = 0; r < rot; r++) {
            int nx = 3 - y;
            int ny = x;

            x = nx;
            y = ny;
        }
        out[i][0] = (int8_t)x;
        out[i][1] = (int8_t)y;
    }
}

/* Rows above the field (r < 0) are allowed, so pieces can enter from the edge */
static int fits(uint8_t b[TH][TW], const int8_t cells[4][2], int bx, int by) {
    for (int i = 0; i < 4; i++) {
        int c = bx + cells[i][0];
        int r = by + cells[i][1];

        if (c < 0 || c >= TW || r >= TH) {
            return 0;
        }
        if (r >= 0 && b[r][c]) {
            return 0;
        }
    }
    return 1;
}

static int clear_lines(uint8_t b[TH][TW]) {
    int n = 0;

    for (int r = TH - 1; r >= 0;) {
        int full = 1;

        for (int c = 0; c < TW; c++) {
            if (!b[r][c]) {
                full = 0;
                break;
            }
        }
        if (full) {
            memmove(&b[1][0], &b[0][0], (size_t)r * TW);
            memset(&b[0][0], 0, TW);
            n++;
        }
        else {
            r--;
        }
    }
    return n;
}

static int eval_board(uint8_t b[TH][TW], int lines) {
    int agg = 0, holes = 0, bump = 0, prev = 0;

    for (int c = 0; c < TW; c++) {
        int first = -1;

        for (int r = 0; r < TH; r++) {
            if (b[r][c]) {
                first = r;
                break;
            }
        }
        int h = (first < 0) ? 0 : TH - first;

        if (first >= 0) {
            for (int r = first + 1; r < TH; r++) {
                if (!b[r][c]) {
                    holes++;
                }
            }
        }
        agg += h;
        if (c > 0) {
            bump += (h > prev) ? h - prev : prev - h;
        }
        prev = h;
    }
    return -510 * agg + 760 * lines - 357 * holes - 184 * bump;
}

static void choose_target(void) {
    int best = -2000000000;
    int brot = cur.rot;
    int bbx = cur.bx;
    int8_t cells[4][2];
    uint8_t tmp[TH][TW];

    for (int rot = 0; rot < 4; rot++) {
        piece_cells(cur.shape, rot, cells);
        for (int bx = -3; bx < TW; bx++) {
            int by = -3;

            if (!fits(board, cells, bx, by)) {
                continue;
            }
            while (fits(board, cells, bx, by + 1)) {
                by++;
            }
            memcpy(tmp, board, sizeof(tmp));

            int ok = 1;

            for (int i = 0; i < 4; i++) {
                int r = by + cells[i][1];
                int c = bx + cells[i][0];

                if (r < 0) {
                    ok = 0;
                    break;
                }
                tmp[r][c] = 1;
            }
            if (!ok) {
                continue;
            }

            int score = eval_board(tmp, clear_lines(tmp));

            /*
             * Prefer the centre when two landing positions have
             * the same board score. This avoids always choosing
             * the first valid position at the left edge.
             */
            int centre_distance = abs((2 * bx + 3) - (TW - 1));
            int best_distance = abs((2 * bbx + 3) - (TW - 1));

            if (score > best ||
                (score == best && centre_distance < best_distance)) {
                best = score;
                brot = rot;
                bbx = bx;
            }
        }
    }
    cur.t_rot = brot;
    cur.t_bx = bbx;
}

static void tetris_reset_board(void) {
    memset(board, 0, sizeof(board));
    t_score = 0;
    t_lines = 0;
    t_over = 0;
}

static void spawn(void) {
    int8_t cells[4][2];

    cur.shape = rnd_range(7);
    cur.rot = 0;
    cur.bx = (TW - 4) / 2;
    cur.by = -3;
    piece_cells(cur.shape, 0, cells);
    if (!fits(board, cells, cur.bx, cur.by)) {
        t_over = OVER_FRAMES;
        t_games++;
        return;
    }
    choose_target();
}

static void tetris_start(void) {
    tetris_reset_board();
    spawn();
}

static void lock_piece(void) {
    int8_t cells[4][2];

    piece_cells(cur.shape, cur.rot, cells);
    for (int i = 0; i < 4; i++) {
        int r = cur.by + cells[i][1];
        int c = cur.bx + cells[i][0];

        if (r < 0) {
            t_over = OVER_FRAMES;
            t_games++;
            return;
        }
        board[r][c] = 1;
    }

    int n = clear_lines(board);

    t_lines += (uint32_t)n;
    t_score += 10 + (uint32_t)(n * n * 100);
    spawn();
}

static void draw_cell(int c, int r) {
    gfx_fill_rect(TX0 + c * CELL_W, TY0 + r * CELL_H,
                  CELL_W, CELL_H, 1);
}

static void tetris_step(void)
{
    int8_t cells[4][2];

    /* Physical OLED boundary: x=0..127, y=0..31. */
    for (int x = 0; x < GFX_W; x++) {
        gfx_pixel(x, 0, 1);
        gfx_pixel(x, GFX_H - 1, 1);
    }
    for (int y = 0; y < GFX_H; y++) {
        gfx_pixel(0, y, 1);
        gfx_pixel(GFX_W - 1, y, 1);
    }

    if (t_over) {
        if (--t_over == 0) {
            tetris_start();
        }
    } else {
        piece_cells(cur.shape, cur.rot, cells);

        /*
         * Reach the selected rotation and column before descending.
         * Otherwise a piece can hit the stack while still travelling
         * horizontally and never reach its selected landing position.
         */
        int aligned = (cur.rot == cur.t_rot && cur.bx == cur.t_bx);

        if (!aligned) {
            int moved = 0;

            if (cur.rot != cur.t_rot) {
                int8_t nc[4][2];
                int nr = (cur.rot + 1) & 3;

                piece_cells(cur.shape, nr, nc);
                if (fits(board, nc, cur.bx, cur.by)) {
                    cur.rot = nr;
                    moved = 1;
                }
            }

            if (!moved && cur.bx != cur.t_bx) {
                piece_cells(cur.shape, cur.rot, cells);
                int nb = cur.bx + ((cur.t_bx > cur.bx) ? 1 : -1);
                if (fits(board, cells, nb, cur.by)) {
                    cur.bx = nb;
                    moved = 1;
                }
            }

            /*
             * If the selected route is blocked, keep the game moving
             * rather than freezing the piece in place.
             */
            if (!moved) {
                piece_cells(cur.shape, cur.rot, cells);
                if (fits(board, cells, cur.bx, cur.by + 1)) {
                    cur.by++;
                } else {
                    lock_piece();
                }
            }
        } else {
            for (int step = 0; step < 3; step++) {
                piece_cells(cur.shape, cur.rot, cells);
                if (fits(board, cells, cur.bx, cur.by + 1)) {
                    cur.by++;
                } else {
                    lock_piece();
                    break;
                }
            }
        }
    }

    /* Compact 10x8 field: pieces fall from top to bottom. */
    for (int r = 0; r < TH; r++) {
        for (int c = 0; c < TW; c++) {
            if (board[r][c]) {
                draw_cell(c, r);
            }
        }
    }

    if (!t_over) {
        piece_cells(cur.shape, cur.rot, cells);
        for (int i = 0; i < 4; i++) {
            int r = cur.by + cells[i][1];
            int c = cur.bx + cells[i][0];

            if (r >= 0 && c >= 0 && c < TW && r < TH) {
                draw_cell(c, r);
            }
        }
    }
}

void saver_tetris_stats(uint32_t *games, uint32_t *total_lines) {
    if (games) {
        *games = t_games;
    }
    if (total_lines) {
        *total_lines = t_lines;
    }
}

/* ---------------- Public API ---------------- */

void saver_start(saver_id_t id) {
    switch (id) {
        case SAVER_SYMBOLS: symbols_start(); break;
        case SAVER_STARS:   stars_start(); break;
        case SAVER_PAD:     pad_start(); break;
        case SAVER_TETRIS:  tetris_start(); break;
        default: break;
    }
}

void saver_step(saver_id_t id) {
    gfx_clear();
    gfx_set_offset(0, 0);
    switch (id) {
        case SAVER_SYMBOLS: symbols_step(); break;
        case SAVER_STARS:   stars_step(); break;
        case SAVER_PAD:     pad_step(); break;
        case SAVER_TETRIS:  tetris_step(); break;
        default: break;
    }
}

const char *saver_name(saver_id_t id) {
    switch (id) {
        case SAVER_SYMBOLS: return "SYMBOLS";
        case SAVER_STARS:   return "STARS";
        case SAVER_PAD:     return "PAD";
        case SAVER_TETRIS:  return "TETRIS";
        default:            return "?";
    }
}

#endif /* CONFIG_BLUERETRO_SYSTEM_PSX_PS2 */
