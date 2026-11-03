/*
 * Wiring and display configuration for the OLED + buzzer UI.
 * Local addition, not part of upstream BlueRetro.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _UI_CONFIG_H_
#define _UI_CONFIG_H_

#define UI_SCL_PIN      17
#define UI_SDA_PIN      18
#define UI_BUZZER_PIN   23
#define UI_OLED_ADDR    0x3C    /* Some modules are 0x3D */
#define UI_OLED_HEIGHT  32      /* 64 or 32 (this module: 128x32) */
#define UI_OLED_FLIP    0       /* Set to 1 if the picture is upside down / mirrored */
#define UI_TITLE        "BLUERETRO PS2 HW2"
#define UI_I2C_DELAY_US 2       /* Lower = faster screen refresh; raise if the picture is garbled */

#endif /* _UI_CONFIG_H_ */
