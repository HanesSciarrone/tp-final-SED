/** \file config.h
 *  Mar 2022
 *  Maestría en SIstemas Embebidos - Sistemas embebidos distribuidos
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/* configuración WIFI */
#define ESP_WIFI_SSID "ssid"     // CONFIG_ESP_WIFI_SSID  // "SSID wifi"
#define ESP_WIFI_PASS "pass"     // CONFIG_ESP_WIFI_PASS  // "pass wifi"

/*  Configuracion MQTT  */
#define PORT_MQTT       1883                    //CONFIG_PORT_MQTT      // default
#define IP_BROKER_MQTT  "broker"                //CONFIG_IP_BROKER_MQTT // Broker MQTT
#define SUBCRIBE_TOPIC  "tp-final/Hanes/App"    // Subcription topic
#define PUBLISH_TOPIC   "tp-final/Hanes/ESP32"  // Publish topic

//#define USER_MQTT "usuario"             // Usuario de MQTT
//#define PASSWD_MQTT "usuariopassword"   // Contraseña de MQTT

/*  configuración IO */
#define BLINK_GPIO CONFIG_BLINK_GPIO  // port 2 para NodeMcu-23S

#define SENSOR_REP          2556
#define SENSOR_MAX          3135
#define MAX_TIME_MEASURE    60
#define SAMPLE_FREQUENCY    100
#define SAMPLE_TIME         0.01

/* Configuración SD  */

/* Configuración CRONO  */

#endif
