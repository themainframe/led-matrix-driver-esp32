#include <string.h>
#include <stdint.h>
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "config.h"

static const char* TAG = "Config";

/**
 * Configuration records.
 */
config_entry_t config[] = {
    {
        .key = CONFIG_WIFI_SSID,
        .value = NULL,
        .default_value = "default_ssid",
        .is_dirty = false
    },
    {
        .key = CONFIG_WIFI_PSK,
        .value = NULL,
        .default_value = "default_psk",
        .is_dirty = false
    },
    {
        .key = CONFIG_GCR,
        .value = NULL,
        .default_value = "255",
        .is_dirty = false
    }
};

size_t config_count = sizeof(config) / sizeof(config_entry_t);

/**
 * Initialise the flash storage.
 */
void flash_init() 
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased - retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
}

/**
 * Load configuration from NVS, if present.
 */
int config_load() 
{   
    flash_init();

    nvs_handle flash_handle;
    esp_err_t err = nvs_open("config", NVS_READWRITE, &flash_handle);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "error (%s) opening NVS handle for loading", esp_err_to_name(err));
        return -1;
    }

    // For each configuration item
    for (int cfg_idx = 0; cfg_idx < sizeof(config) / sizeof(config_entry_t); cfg_idx ++) {

        // Allocate space for each configuration value
        size_t value_size;
        err = nvs_get_str(flash_handle, config[cfg_idx].key, NULL, &value_size);

        // Does the value just not exist yet?
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "value for config item %s not found in NVS storage", config[cfg_idx].key);
            continue;
        }

        ESP_ERROR_CHECK(err);
        config[cfg_idx].value = malloc(value_size);

        // Read the value
        err = nvs_get_str(flash_handle, config[cfg_idx].key, config[cfg_idx].value, &value_size);
        ESP_ERROR_CHECK(err);
    }

    // Finished reading flash data, close the handle
    nvs_close(flash_handle);
    return 0;
}

/**
 * Save configuration to NVS.
 */
int config_save()
{
    flash_init();

    nvs_handle flash_handle;
    esp_err_t err = nvs_open("config", NVS_READWRITE, &flash_handle);

    if (err != ESP_OK) {
        ESP_LOGI(TAG, "error (%s) opening NVS handle for saving", esp_err_to_name(err));
        return -1;
    }

    // For each configuration item
    for (int cfg_idx = 0; cfg_idx < sizeof(config) / sizeof(config_entry_t); cfg_idx ++) {
        
        // Only write dirty entries to NVS
        if (!config[cfg_idx].is_dirty) {
            continue;
        }

        // Write the value (or the default if not yet set)
        err = nvs_set_str(
            flash_handle,
            config[cfg_idx].key,
            config[cfg_idx].value != NULL ? config[cfg_idx].value : config[cfg_idx].default_value
        );

        ESP_ERROR_CHECK(err);
    }

    // Commit
    err = nvs_commit(flash_handle);
   if (err != ESP_OK) {
        ESP_LOGI(TAG, "error (%s) committing NVS", esp_err_to_name(err));
        return -2;
    }

    // Finished reading flash data, close the handle
    nvs_close(flash_handle);

    return ESP_OK;
}

/**
 * Find a configuration item by key, returning a pointer to it.
 */
config_entry_t* config_find(const char* key)
{
    for (int cfg_idx = 0; cfg_idx < sizeof(config) / sizeof(config_entry_t); cfg_idx ++) {
        if (strcmp(config[cfg_idx].key, key) == 0) {
            return &config[cfg_idx];
        }
    }

    return NULL;
}

/**
 * Update a configuration value.
 * Note: configuration is only persisted to storage upon config_save() being called.
 */
int config_set(const char* key, const char* value)
{
    // Find the item to update
    config_entry_t* config_entry = config_find(key);

    if (config_entry == NULL) {
        ESP_LOGW(TAG, "requested update of non-existent configuration entry: %s", key);
        return -1;
    }

    // Free the previous value if one is present
    if (config_entry->value != NULL) {
        free(config_entry->value);
    }

    // Allocate a new string and copy the new value into it
    config_entry->value = malloc(strlen(value));
    strcpy(config_entry->value, value);
    config_entry->is_dirty = true;

    return 0;
}

/**
 * Retrieve a configuration value.
 */
const char* config_get(const char* key)
{
    // Find the item
    config_entry_t* config_entry = config_find(key);

    if (config_entry == NULL) {
        ESP_LOGW(TAG, "requested non-existent configuration entry: %s", key);
        return NULL;
    }

    if (config_entry->value == NULL) {
        return config_entry->default_value;
    }

    return config_entry->value;
}

/**
 * Retrieve a configuration value as an integer.
 */
int config_get_int(const char* key)
{
    const char* value = config_get(key);
    if (!value) {
        return -1;
    }

    return atoi(value);
}