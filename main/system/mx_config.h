#ifndef __MX_CONFIG_H__
#define __MX_CONFIG_H__

#include <stdint.h>
#include <stddef.h>

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

enum mx_cfg_slot {
    MX_SLOT_AUDIO = 0,
    MX_SLOT_OLED,
    MX_SLOT_LED,
    MX_SLOT_IDLE,
    MX_SLOT_TIME,
    MX_SLOT_PAGES,
    MX_SLOT_FEATURES,
    MX_SLOT_LAST,
};

struct mx_audio_cfg {
    uint8_t enabled;
    uint8_t volume;
    uint8_t ev_power_on;
    uint8_t ev_power_off;
    uint8_t ev_reset;
    uint8_t ev_bt_connect;
    uint8_t ev_bt_disconnect;
    uint8_t rsvd;
} __packed;

#define MX_OLED_PAGES_MAX 8
#define MX_OLED_PAGE_EMPTY 0xFF

struct mx_oled_cfg {
    uint8_t enabled;
    uint8_t rotation;
    uint8_t panel_h;
    uint8_t notifications;
    uint8_t touchpad_nav;
    uint8_t page_cnt;
    uint8_t pages[MX_OLED_PAGES_MAX];
} __packed;

#define MX_LED_PORTS 8

struct mx_led_cfg {
    uint8_t auto_player;
    uint8_t brightness;
    struct {
        uint8_t r, g, b;
    } ports[MX_LED_PORTS];
} __packed;

enum mx_idle_mode {
    MX_IDLE_OFF = 0,
    MX_IDLE_CUBE,
    MX_IDLE_STARS,
    MX_IDLE_TETRIS,
    MX_IDLE_PS_SYMBOLS,
    MX_IDLE_BALLS,
    MX_IDLE_PONG,
    MX_IDLE_CLOCK_ANALOG,
    MX_IDLE_CLOCK_FLIP,
    MX_IDLE_CLOCK_RECT,
};

struct mx_idle_cfg {
    uint8_t mode;
    uint16_t idle_timeout;
} __packed;

struct mx_time_cfg {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
} __packed;

struct mx_features_cfg {
    uint8_t bits;
} __packed;

void mx_config_init(void);
void mx_config_select_slot(uint8_t slot);
uint32_t mx_config_read_blob(uint8_t *dst, uint32_t max);
int32_t mx_config_write_blob(const uint8_t *src, uint32_t len);

const struct mx_audio_cfg *mx_cfg_audio(void);
const struct mx_oled_cfg *mx_cfg_oled(void);
const struct mx_led_cfg *mx_cfg_led(void);
const struct mx_idle_cfg *mx_cfg_idle(void);
const struct mx_time_cfg *mx_cfg_time(void);
const struct mx_features_cfg *mx_cfg_features(void);
void mx_cfg_set_time(const struct mx_time_cfg *t);

#endif
