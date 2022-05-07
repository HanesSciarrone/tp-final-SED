/** \file	wifi.h
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 */

#ifndef WIFI_H_
#define WIFI_H_

/* Prototipos */
void WIFI_init();
void WIFI_userInit(const char * , const char * );
int8_t WIFI_getRSSI();
int WIFI_getIP(char * );

/* Definiciones */
#define EXAMPLE_ESP_MAXIMUM_RETRY  10
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define LENGTH_STR_IP 15

#define MAX_WIFI_SSID_LENGTH 32
#define MAX_WIFI_PASS_LENGTH 64

#endif /* WIFI_H_ */
