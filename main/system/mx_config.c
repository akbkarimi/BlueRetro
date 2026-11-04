#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "mx_config.h"

#define MX_NVS_NS "mx_cfg"

static struct mx_audio_cfg audio_cfg;
static struct mx_oled_cfg oled_cfg;
static struct mx_led_cfg led_cfg;
static struct mx_idle_cfg idle_cfg;
static struct mx_time_cfg time_cfg;
static struct mx_features_cfg features_cfg;

static uint8_t selected_slot = MX_SLOT_AUDIO;

static void mx_config_defaults(void)
{
    static const uint8_t led_defaults[MX_LED_PORTS][3] = {
        {0x00, 0x44, 0xFF},
        {0xFF, 0x00, 0x44},
        {0x00, 0xDD, 0x44},
        {0xFF, 0xCC, 0x00},
        {0xFF, 0x66, 0x00},
        {0xAA, 0x00, 0xFF},
        {0xFF, 0xFF, 0xFF},
        {0x00, 0xCC, 0xFF},
    };

    memset(&audio_cfg, 0, sizeof(audio_cfg));
    audio_cfg.enabled = 1;
    audio_cfg.volume = 70;
    audio_cfg.ev_power_on = 2;
    audio_cfg.ev_power_off = 1;
    audio_cfg.ev_reset = 3;
    audio_cfg.ev_bt_connect = 6;
    audio_cfg.ev_bt_disconnect = 1;

    memset(&oled_cfg, MX_OLED_PAGE_EMPTY, sizeof(oled_cfg));
    oled_cfg.enabled = 1;
    oled_cfg.rotation = 0;
    oled_cfg.panel_h = 64;
    oled_cfg.notifications = 1;
    oled_cfg.touchpad_nav = 1;
    oled_cfg.page_cnt = 5;

    for (uint8_t i = 0; i < oled_cfg.page_cnt; i++) {
        oled_cfg.pages[i] = i;
    }

    memset(&led_cfg, 0, sizeof(led_cfg));
    led_cfg.auto_player = 1;
    led_cfg.brightness = 80;
    memcpy(led_cfg.ports, led_defaults, sizeof(led_defaults));

    idle_cfg.mode = MX_IDLE_CUBE;
    idle_cfg.idle_timeout = 30;

    memset(&time_cfg, 0, sizeof(time_cfg));

    features_cfg.bits = 0x7F;
}

static bool mx_nvs_load(const char *key, void *dst, size_t expected)
{
    nvs_handle_t h;
    size_t len = expected;

    if (nvs_open(MX_NVS_NS, NVS_READONLY, &h) != ESP_OK) {
        return false;
    }

    esp_err_t err = nvs_get_blob(h, key, dst, &len);
    nvs_close(h);

    return err == ESP_OK && len == expected;
}

static bool mx_nvs_save(const char *key, const void *src, size_t len)
{
    nvs_handle_t h;

    if (nvs_open(MX_NVS_NS, NVS_READWRITE, &h) != ESP_OK) {
        return false;
    }

    esp_err_t err = nvs_set_blob(h, key, src, len);

    if (err == ESP_OK) {
        err = nvs_commit(h);
    }

    nvs_close(h);
    return err == ESP_OK;
}

static void mx_config_load_all(void)
{
    struct mx_audio_cfg a;
    struct mx_oled_cfg o;
    struct mx_led_cfg l;
    struct mx_idle_cfg i;
    struct mx_features_cfg f;

    if (mx_nvs_load("audio", &a, sizeof(a)) &&
        a.enabled <= 1 && a.volume <= 100) {
        audio_cfg = a;
    }

    uint8_t oled_data[sizeof(struct mx_oled_cfg)];
    nvs_handle_t h;

    if (nvs_open(MX_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        size_t len = sizeof(oled_data);
        esp_err_t err = nvs_get_blob(h, "oled", oled_data, &len);
        nvs_close(h);

        if (err == ESP_OK && len >= 6 &&
            len <= sizeof(oled_data) &&
            oled_data[0] <= 1 &&
            oled_data[1] <= 3 &&
            (oled_data[2] == 32 || oled_data[2] == 64) &&
            oled_data[3] <= 1 &&
            oled_data[4] <= 1 &&
            oled_data[5] <= MX_OLED_PAGES_MAX &&
            len == (size_t)(6 + oled_data[5])) {
            struct mx_oled_cfg tmp = oled_cfg;
            memcpy(&tmp, oled_data, len);
            oled_cfg = tmp;
        }
    }

    if (mx_nvs_load("led", &l, sizeof(l)) &&
        l.auto_player <= 1 && l.brightness <= 100) {
        led_cfg = l;
    }

    if (mx_nvs_load("idle", &i, sizeof(i)) &&
        i.mode <= MX_IDLE_CLOCK_RECT) {
        idle_cfg = i;
    }

    if (mx_nvs_load("feat", &f, sizeof(f))) {
        features_cfg = f;
    }
}

void mx_config_init(void)
{
    mx_config_defaults();
    selected_slot = MX_SLOT_AUDIO;
    mx_config_load_all();
}

void mx_config_select_slot(uint8_t slot)
{
    if (slot < MX_SLOT_LAST && slot != MX_SLOT_PAGES) {
        selected_slot = slot;
    }
}

uint32_t mx_config_read_blob(uint8_t *dst, uint32_t max)
{
    const void *src = NULL;
    uint32_t len = 0;

    switch (selected_slot) {
    case MX_SLOT_AUDIO:
        src = &audio_cfg;
        len = sizeof(audio_cfg);
        break;

    case MX_SLOT_OLED:
        src = &oled_cfg;
        len = 6 + oled_cfg.page_cnt;
        break;

    case MX_SLOT_LED:
        src = &led_cfg;
        len = sizeof(led_cfg);
        break;

    case MX_SLOT_IDLE:
        src = &idle_cfg;
        len = sizeof(idle_cfg);
        break;

    case MX_SLOT_TIME:
        src = &time_cfg;
        len = sizeof(time_cfg);
        break;

    case MX_SLOT_FEATURES:
        src = &features_cfg;
        len = sizeof(features_cfg);
        break;

    default:
        return 0;
    }

    if (!dst || max < len) {
        return 0;
    }

    memcpy(dst, src, len);
    return len;
}

int32_t mx_config_write_blob(const uint8_t *src, uint32_t len)
{
    if (!src) {
        return -1;
    }

    switch (selected_slot) {
    case MX_SLOT_AUDIO: {
        struct mx_audio_cfg tmp;

        if (len != sizeof(tmp)) {
            return -1;
        }

        memcpy(&tmp, src, sizeof(tmp));

        if (tmp.enabled > 1 || tmp.volume > 100) {
            return -1;
        }

        if (!mx_nvs_save("audio", &tmp, sizeof(tmp))) {
            return -1;
        }

        audio_cfg = tmp;
        return 0;
    }

    case MX_SLOT_OLED: {
        struct mx_oled_cfg tmp = oled_cfg;

        if (len < 6 || len > sizeof(tmp) ||
            src[0] > 1 || src[1] > 3 ||
            (src[2] != 32 && src[2] != 64) ||
            src[3] > 1 || src[4] > 1 ||
            src[5] > MX_OLED_PAGES_MAX ||
            len != (uint32_t)(6 + src[5])) {
            return -1;
        }

        memcpy(&tmp, src, len);

        if (!mx_nvs_save("oled", &tmp, len)) {
            return -1;
        }

        oled_cfg = tmp;
        return 0;
    }

    case MX_SLOT_LED: {
        struct mx_led_cfg tmp;

        if (len != sizeof(tmp)) {
            return -1;
        }

        memcpy(&tmp, src, sizeof(tmp));

        if (tmp.auto_player > 1 || tmp.brightness > 100) {
            return -1;
        }

        if (!mx_nvs_save("led", &tmp, sizeof(tmp))) {
            return -1;
        }

        led_cfg = tmp;
        return 0;
    }

    case MX_SLOT_IDLE: {
        struct mx_idle_cfg tmp;

        if (len != sizeof(tmp)) {
            return -1;
        }

        memcpy(&tmp, src, sizeof(tmp));

        if (tmp.mode > MX_IDLE_CLOCK_RECT) {
            return -1;
        }

        if (!mx_nvs_save("idle", &tmp, sizeof(tmp))) {
            return -1;
        }

        idle_cfg = tmp;
        return 0;
    }

    case MX_SLOT_TIME: {
        struct mx_time_cfg tmp;

        if (len != sizeof(tmp)) {
            return -1;
        }

        memcpy(&tmp, src, sizeof(tmp));
        time_cfg = tmp;
        return 0;
    }

    case MX_SLOT_FEATURES: {
        struct mx_features_cfg tmp;

        if (len != sizeof(tmp)) {
            return -1;
        }

        memcpy(&tmp, src, sizeof(tmp));

        if (!mx_nvs_save("feat", &tmp, sizeof(tmp))) {
            return -1;
        }

        features_cfg = tmp;
        return 0;
    }

    case MX_SLOT_PAGES:
    default:
        return -1;
    }
}

const struct mx_audio_cfg *mx_cfg_audio(void)
{
    return &audio_cfg;
}

const struct mx_oled_cfg *mx_cfg_oled(void)
{
    return &oled_cfg;
}

const struct mx_led_cfg *mx_cfg_led(void)
{
    return &led_cfg;
}

const struct mx_idle_cfg *mx_cfg_idle(void)
{
    return &idle_cfg;
}

const struct mx_time_cfg *mx_cfg_time(void)
{
    return &time_cfg;
}

const struct mx_features_cfg *mx_cfg_features(void)
{
    return &features_cfg;
}

void mx_cfg_set_time(const struct mx_time_cfg *t)
{
    if (t) {
        time_cfg = *t;
    }
}
