/*
 * OLED screensavers (PlayStation symbols, stars, PS pad, Tetris demo).
 * Draws into the ui_gfx framebuffer, no hardware access.
 * Local addition, not part of upstream BlueRetro.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UI_SAVER_H_
#define _UI_SAVER_H_

#include <stdint.h>

typedef enum {
    SAVER_SYMBOLS = 0,  /* Bouncing cross, circle, triangle, square and star */
    SAVER_STARS,        /* Scrolling starfield */
    SAVER_PAD,          /* Bouncing PlayStation style controller */
    SAVER_TETRIS,       /* Tetris playing itself */
    SAVER_COUNT,
} saver_id_t;

void saver_seed(uint32_t seed);
void saver_start(saver_id_t id);            /* Reset that saver's animation */
void saver_step(saver_id_t id);             /* Clear the framebuffer and draw one frame */
const char *saver_name(saver_id_t id);
void saver_tetris_stats(uint32_t *games, uint32_t *total_lines);   /* For debugging */

#endif /* _UI_SAVER_H_ */
