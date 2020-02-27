#ifndef EVENTS_H
#define EVENTS_H

#include "freertos/event_groups.h"

// Event group for system events
EventGroupHandle_t sys_event_group;

#define SYS_EVENT_BIT_WIFI_CONNECTED 0x01
#define SYS_EVENT_BIT_WIFI_DISCONNECTED 0x02

#endif