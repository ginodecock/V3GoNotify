#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

#define MQTT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#define MQTT_HOST     "api.allthingstalk.io" //or "mqtt.yourdomain.com"
#define MQTT_PORT     1883
#define MQTT_BUF_SIZE   1024
#define MQTT_KEEPALIVE    120  /*second*/

#define MQTT_CLIENT_ID    "Kx7vo0kSYI5P6e9PrJIOKWS7"
#define MQTT_USER     "GoNotify:GoNotify"
#define MQTT_PASS     "5bjsg52pzc2"
#define MQTT_CLEAN_SESSION 1
#define MQTT_KEEPALIVE 120

#define STA_SSID "KlokjeGo"
#define STA_PASS "ZwaluWBos"

#define MQTT_RECONNECT_TIMEOUT  5 /*second*/

#define DEFAULT_SECURITY  0
#define QUEUE_BUFFER_SIZE       2048

#define PROTOCOL_NAMEv31  /*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311     /*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

#if defined(DEBUG_ON)
#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#else
#define INFO( format, ... )
#endif

#endif // __MQTT_CONFIG_H__
