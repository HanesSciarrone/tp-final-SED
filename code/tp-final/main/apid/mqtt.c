/** \file	mqtt.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene las funciones de inicializacion y manejo del protocolo mqtt
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

/* APID */
#include "../config.h"
#include "../include/wifi.h"
#include "../include/io.h"
#include "../include/mqtt.h"
#include "../common.h"

/* TAGs */
static const char *TAG = "MQTT";

#define INDEX_TIME_MESSAGE  8

/********************************** MQTT **************************************/

/* Definiciones */
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t);
static void mqtt_event_handler(void *, esp_event_base_t , int32_t , void *);
const char* mqtt_broker_default = IP_BROKER_MQTT;
const int mqttPort = PORT_MQTT;
//const char* mqttUser = USER_MQTT;
//const char* mqttPassword = PASSWD_MQTT;
static esp_mqtt_client_handle_t client;
esp_mqtt_client_handle_t MQTT_getClient(void);

// parámetros Wifi
//extern wifi_ap_record_t wifidata;

extern volatile unsigned char startMeasure;
extern volatile int measureDuration;
extern volatile uint32_t sizeSample;
extern volatile uint8_t samplesReady;
/*******************************************************************************
MQTT_subscribe(): Subscripción al topic especificado.
*******************************************************************************/
void MQTT_subscribe(const char * topic){

  esp_mqtt_client_subscribe(client, topic, 0);

}

/*******************************************************************************
 MQTT_publish(): publica mensaje en el topic especificado.
*******************************************************************************/
void MQTT_publish(const char * topic, const char * mensaje) {

  /* CON PUPLISH *********************************************************
  esp_mqtt_client_publish(client, topic, data, len, qos, retain) */
  esp_mqtt_client_publish(client, topic, mensaje, 0, 0, 0);

  /* CON ENQUEUE ********************************************************
  esp_mqtt_client_enqueue(client, topic, data, len, qos, retain, store);
  //esp_mqtt_client_enqueue(client, topic, mensaje, 0, 0, 0, 1);*/
}


/*******************************************************************************
 MQTT_processTopic(): lee el mensaje MQTT recibido cuando se dispara el evento
 ******************************************************************************/
 void MQTT_processTopic(const char * topic, const char * msg){
    /* Acciones a ejecutar para cada topic recibido */
    if (strcmp(SUBCRIBE_TOPIC, topic)  != 0) {
        return;
    }

    /*
     * Format string are:
     *
     * - Start = time in seconds. For example: Start = 45
     * - Stop
    */
    if (strstr(msg, "Start = ") != NULL ) {
        if (!isNumber(msg + INDEX_TIME_MESSAGE)) {
            measureDuration = MAX_TIME_MEASURE;
        }
        else {
            measureDuration = atoi(msg + INDEX_TIME_MESSAGE);

            if (measureDuration < 0 || measureDuration > MAX_TIME_MEASURE) {
                measureDuration = MAX_TIME_MEASURE;
            }
        }

        ESP_LOGI(TAG, "Duration is = %d", measureDuration);
        saveLog(INPUT, topic, msg);
        startMeasure = 1;
    }
    else if (!strcmp(msg, "Stop")) {
        saveLog(INPUT, topic, msg);
        startMeasure = 0;
        sizeSample = 0;
        samplesReady = 0;
    }

 }
 /*******************************************************************************
  MQTT_init(): Inicialización de MQTT
  ******************************************************************************/
 void MQTT_init(){

  MQTT_userInit(mqtt_broker_default);

 }

 /*******************************************************************************
 MQTT_userIinit(): Inicialización de MQTT definida por el usuario desde un archivo
  ******************************************************************************/
 void MQTT_userInit(const char * mqtt_broker){

   esp_mqtt_client_config_t mqtt_cfg = {
           //.uri = CONFIG_BROKER_URL,
           .host = mqtt_broker,//server,
           //.username = mqttUser,
           //.password = mqttPassword,
           .port = mqttPort,
   };


   ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
   client = esp_mqtt_client_init(&mqtt_cfg); //   Creates mqtt client handle based on the configuration.
   esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
   esp_mqtt_client_start(client); // Starts mqtt client with already created client handle.
   vTaskDelay(50 / portTICK_PERIOD_MS); // waiting 50 ms

 }

 /* handle cliente MQTT ************************************************/
 esp_mqtt_client_handle_t MQTT_getClient(void)
 {
         return client;
 }


/*****************************************************************************/
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
        //esp_mqtt_client_handle_t client = event->client;
        client = event->client;

        // your_context_t *context = event->context;
        switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
                break;

        case MQTT_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
                break;

        case MQTT_EVENT_SUBSCRIBED:
                ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
                break;

        case MQTT_EVENT_UNSUBSCRIBED:
                ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
                break;

        case MQTT_EVENT_PUBLISHED:
                ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
                break;

        case MQTT_EVENT_DATA:   // Cuando recibo un mensaje a un topic que estoy subcripto.
                ESP_LOGI(TAG, "MQTT_EVENT_DATA");

                // obtener topic y mensaje recibido
                char rcv_topic[MAX_TOPIC_LENGTH]="";
                char rcv_message[MAX_MSG_LENGTH]="";
                strncpy(rcv_topic, event->topic, event->topic_len);
                strncpy(rcv_message, event->data, event->data_len);
                //ESP_LOGI(TAG, "TOPIC RECEIVED: %s", rcv_topic );
                //ESP_LOGI(TAG, "MESSAGE RECEIVED: %s", rcv_message);
                MQTT_processTopic(rcv_topic, rcv_message);
                break;

        case MQTT_EVENT_ERROR:
                ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
                break;

        default:
                ESP_LOGI(TAG, "Other event id:%d", event->event_id);
                break;
        }
        return ESP_OK;
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
        ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
        mqtt_event_handler_cb(event_data);
}
