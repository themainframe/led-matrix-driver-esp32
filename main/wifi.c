#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "events.h"
#include "wifi.h"
#include "config.h"

static const char *TAG = "Wi-Fi";

// Has the event loop initialisation been done?
static bool event_loop_init_done =  false;

// The system event group
extern EventGroupHandle_t sys_event_group;

/**
 * Event handler for the various events generated during Wi-Fi initialisation.
 */
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {

        // Station started up
        case SYSTEM_EVENT_STA_START:
            ESP_LOGI(TAG, "STA startup completed - connecting...");
            esp_wifi_connect();
            break;

        // Station got IP address
        case SYSTEM_EVENT_STA_GOT_IP:
            // Set the relevant bit in our event group that's being waited on by the main call
            ESP_LOGI(TAG, "station got IP address: %s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            xEventGroupSetBits(sys_event_group, SYS_EVENT_BIT_WIFI_CONNECTED);
            break;

        // Station disconnected
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "disconnected from the AP");
            xEventGroupSetBits(sys_event_group, SYS_EVENT_BIT_WIFI_DISCONNECTED);
            break;

        default:
            break;
    }
    return ESP_OK;
}

/**
 * Initialise Wi-Fi.
 */
void wifi_init()
{
    // Create the event group for Wi-Fi state change events
    sys_event_group = xEventGroupCreate();
    
    // Initialse TCP/IP stack
    ESP_LOGI(TAG, "initialising TCP/IP stack...");
    tcpip_adapter_init();

    // Start event loop waiting for Wi-Fi initialisation events
    if (!event_loop_init_done) {
        ESP_LOGI(TAG, "starting event loop...");
        ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
        event_loop_init_done = true;
    }

    // Configure the Wi-Fi
    ESP_LOGI(TAG, "setting default Wi-Fi parameters...");
    wifi_init_config_t wifi_config_default = WIFI_INIT_CONFIG_DEFAULT();
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config_default));
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
    
    // Set max power
    esp_wifi_set_max_tx_power(7);

    // Load SSID/key from NVS-backed config
    const char* config_ssid = config_get(CONFIG_WIFI_SSID);
    const char* config_psk = config_get(CONFIG_WIFI_PSK);

    // Setup Wi-Fi
    wifi_config_t wifi_config;
    
    // Copy Wi-Fi details into the wifi_config_t fields
    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char*)&wifi_config.sta.ssid, config_ssid);
    strcpy((char*)&wifi_config.sta.password, config_psk);

    ESP_LOGI(TAG, "setting Wi-Fi parameters (SSID: %s, PSK: %s)...", wifi_config.sta.ssid, wifi_config.sta.password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    
    ESP_LOGI(TAG, "starting Wi-Fi...");
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * Shut down Wi-Fi connectivity.
 */
void wifi_stop()
{
    ESP_LOGI(TAG, "stopping Wi-Fi...");
    esp_wifi_stop();
}