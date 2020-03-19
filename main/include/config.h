#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stddef.h>

// Config key names
#define CONFIG_WIFI_SSID "wifi_ssid"
#define CONFIG_WIFI_PSK "wifi_psk"
#define CONFIG_GCR "display_gcr"

typedef struct {
    char* key;
    char* value;
    const char* default_value;
    bool is_dirty;
} config_entry_t;

extern config_entry_t config[];
extern size_t config_count;

// Methods
int config_load();
int config_save();
const char* config_get(const char* key);
int config_get_int(const char* key);
int config_set(const char* key, const char* value);

#endif