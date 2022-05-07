/** \file	wifi.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene funciones para configuración y manejo de Wifi.
 *
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

/* APID */
#include "../config.h"
#include "../include/wifi.h"
#include "../include/sd.h"

/* TAGs */
static const char *TAG = "WIFI";

/********************************* WIFI ***************************************/

/* Definiciones */
wifi_ap_record_t wifidata;
static int s_retry_num = 0;
static void event_handler(void* , esp_event_base_t, int32_t, void* );
static char ip_addr[LENGTH_STR_IP+1]="0.0.0.0"; // extern char dir_ip[20];
const char probar[10]="prueba";
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;


/*******************************************************************************
WIFI_getIP(): devuelve por referencia la Ip asignada al dispositivo.
Si aún no fue asignada, devuelve 0, sino 1.
*******************************************************************************/
int WIFI_getIP(char * ip){

  strncpy(ip, ip_addr, strlen(ip_addr));

  if(strcmp(ip_addr,"0.0.0.0")==0){
    return 0; // aún no se obtuvo la dirección ip
  }
  else {
    return 1; // dirección ip ya asignada
  }

}

/******************************************************************************
WIFI_getRSSI(): lee el nivel de potencia de señal (RSSI [dBm]) de la red WiFi a
la que está conectado el dispositivo
*******************************************************************************/
int8_t WIFI_getRSSI(){

  esp_wifi_sta_get_ap_info(&wifidata);

  return wifidata.rssi;

}

/******************************************************************************
WIFI_init(): conexión a la WiFi especificada según la configuración de SSID y PASS
Esta inicialización toma los parámetros por defectos definidos en "config.h"
*******************************************************************************/
void WIFI_init(){

  // ssid y pass definidos por defecto en "config.h"
  printf("SSID VER: %s", ESP_WIFI_SSID);
  WIFI_userInit(ESP_WIFI_SSID, ESP_WIFI_PASS);

}

/******************************************************************************
WIFI_userInit(): conexión a la WiFi especificada según la configuración de SSID y PASS
tomando como parámetros los definidos por el usuario (por ejemplo desde la memoria SD).
*******************************************************************************/
void WIFI_userInit(const  char * ssid, const  char * pass){
    printf("SSID VER: %s", ssid);
    //Initialize NVS (pre Connect)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Connect
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
            .sta = {
                    .ssid = ESP_WIFI_SSID,      // por defecto
                    .password = ESP_WIFI_PASS,  // por defecto
                    .pmf_cfg = {
                            .capable = true,
                            .required = false
                    },
            },
    };

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid)+1);
    memcpy(wifi_config.sta.password, pass, strlen(pass)+1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", ESP_WIFI_SSID, ESP_WIFI_PASS);
    } else {
            ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    vEventGroupDelete(s_wifi_event_group);
}



/******************************************************************************/
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
                esp_wifi_connect();
        } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
                if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
                        esp_wifi_connect();
                        s_retry_num++;
                        ESP_LOGI(TAG, "retry to connect to the AP");
                } else {
                        xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                }
                ESP_LOGI(TAG,"connect to the AP fail");
        } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
                ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
                s_retry_num = 0;
                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

                // Inicializo la variable de la dirección IP para la identificación del nodo
                sprintf(ip_addr, IPSTR, IP2STR(&event->ip_info.ip));
                ESP_LOGI(TAG, "Ip: %s\r\n", ip_addr);

                if (esp_wifi_sta_get_ap_info(&wifidata)==0) {
                ESP_LOGI(TAG, "RSSI: %d\r\n", wifidata.rssi);
                }
        }

}
