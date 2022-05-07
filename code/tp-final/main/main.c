#include <stdint.h>
#include "config.h"
#include "include/crono.h"
#include "include/io.h"
#include "main.h"
#include "common.h"

const char *TAG = "PRACTICA-FINAL";
volatile uint32_t data[MAX_TIME_MEASURE*SAMPLE_FREQUENCY] = {0};    // Lectura del ADC en el instante actual
volatile uint32_t sizeSample = 0;                                   // Quantity of sample.
volatile uint8_t samplesReady = FALSE;                              // Indicate when the data is ready to store
volatile unsigned char startMeasure = 0;
volatile int measureDuration = 60;

char config_ssid[32]    = {0};
char config_pass[32]    = {0};
char config_broker[32]  = {0};

static char setUpParameters(void);
static void createCSVFile(const uint32_t *data, uint32_t size);
/*******************************************************************************
 Programa principal
******************************************************************************/
void app_main(void)
{
    // Inicializaciones
    if (!setUpParameters()) {
        return;
    }

    IO_adcInit();
    IO_gpioInit();
    CRONO_timerInit();
    CRONO_timerStart(SAMPLE_TIME*1000);
    WIFI_userInit(config_ssid, config_pass);
    MQTT_userInit(config_broker);
    CRONO_sntpInit();

    /* Subcription topic */
    MQTT_subscribe(SUBCRIBE_TOPIC);

    while(1) {

        if (samplesReady) {
            samplesReady = 0;
            createCSVFile((const uint32_t *)data, sizeSample);
            sizeSample = 0;
        }

        CRONO_delayMs(500);
    }

    SD_unmount();
}

static char setUpParameters(void)
{
    FILE *fd = NULL;

    // ============= Setting connection parameters =============
    SD_mount();
    fd = SD_open("config.txt", "r");

    if (fd == NULL) {
        ESP_LOGE(TAG, "Problem opening configuration file.\n");
        SD_unmount();
        return FALSE;
    }

    SD_gets(fd, config_ssid, sizeof(config_ssid));
    ESP_LOGI(TAG, "config_ssid = %s", config_ssid);

    SD_gets(fd, config_pass, sizeof(config_pass));
    ESP_LOGI(TAG, "config_pass = %s", config_pass);

    SD_gets(fd, config_broker, sizeof(config_broker));
    ESP_LOGI(TAG, "config_broker = %s", config_broker);

    SD_close(fd);
    fd = NULL;
    ESP_LOGI(TAG, "Module configured.");
    // =========================================================

    return  TRUE;
}

void compareThreshold(void)
{
    int umbral0 = (int)(SENSOR_REP + (SENSOR_MAX-SENSOR_REP)/4.0);
    int umbral1 = (int)(SENSOR_REP + (SENSOR_MAX-SENSOR_REP)/2.0);
    int umbral2 = (int)(SENSOR_REP + (SENSOR_MAX-SENSOR_REP)/4.0*3);

    if(umbral0<fabs(data[sizeSample]) && fabs(data[sizeSample])<umbral1) {
        MQTT_publish(PUBLISH_TOPIC, "0");
        saveLog(OUTPUT, PUBLISH_TOPIC, "ATENCION");
    }
    else if(umbral1<fabs(data[sizeSample]) && fabs(data[sizeSample])<umbral2) {
        MQTT_publish(PUBLISH_TOPIC, "1");
        saveLog(OUTPUT, PUBLISH_TOPIC, "PRECAUCION");
    }
    else if(fabs(data[sizeSample])>umbral2) {
        MQTT_publish(PUBLISH_TOPIC, "2");
        saveLog(OUTPUT, PUBLISH_TOPIC, "ALERTA");
    }
    else{
        ESP_LOGI(TAG, "REPOSO");
    }
}

static void createCSVFile(const uint32_t *data, uint32_t size)
{
    FILE *fd = NULL;
    uint32_t i = 0;
    char timestamp[64] = {0};
    char nameFile[70]= {0};

    snprintf(nameFile, sizeof(nameFile), "A%lli.csv", CRONO_getTime(timestamp, sizeof(timestamp)));
    saveLog(SIMPLE_LOG, "Creating file with name: ", (const char *)nameFile);

    fd = SD_open(nameFile, "w");

    if (fd == NULL) {
        ESP_LOGE(TAG, "Problem to create file %s", nameFile);
        return;
    }

    SD_printf(fd, "Time (ms), Data\n");

    for (i = 0; i < size; i++) {
        SD_printf(fd, "%.3f,%u\n", i*SAMPLE_TIME, data[i]);
    }

    SD_close(fd);
}

