/** \file	io.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene funciones principalmente para el manejo de distintos periféricos */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

/* APID */
#include "../config.h"
#include "../include/io.h"

/* TAGs */
static const char *TAG = "IO/GPIO";


/************************************* ADC ***********************************/
#define DEFAULT_VREF  1100
static esp_adc_cal_characteristics_t *adc_chars;
#if CONFIG_IDF_TARGET_ESP32
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
#elif CONFIG_IDF_TARGET_ESP32S2
static const adc_channel_t channel = ADC_CHANNEL_6;     // GPIO7 if ADC1, GPIO17 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_13;
#endif
static const adc_atten_t atten = ADC_ATTEN_DB_11;//ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

#define LENGTH_PRINT_ADC 80 // Nro de caracteres máximos para stem() desde monitor


/*******************************************************************************
 IO_adcInit(): Inicialización del conversor analógico digital
*******************************************************************************/
void IO_adcInit(){

  adc1_config_width(width);
  adc1_config_channel_atten(channel, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

}

/*******************************************************************************
 IO_readAdc(): Devuelve el valor leido como un entero de 12-bits de resolución
*******************************************************************************/
uint16_t IO_readAdc(){

  return (uint16_t)adc1_get_raw((adc1_channel_t)channel);  // entero 12 bitmeasure;

}

/*******************************************************************************
 IO_voltAdc(): Devuelve la lectura del AD en milivolts
*******************************************************************************/
uint32_t IO_voltAdc(){

  uint32_t measure;
  esp_adc_cal_get_voltage((adc1_channel_t)channel, adc_chars, &measure);
  return measure;

}

/*******************************************************************************
 IO_printAdc(): imprime en el monitor serie un string de caracteres (por defecto '*')
 cuya cantidad es proporcional al valor x sensado por el conversor de 12-bits.
 Depende de la constante LENGTH_PRINT_ADC, en donde se especifica cuántos caracteres
 debe abarcar el máximo valor de x (4095).
*******************************************************************************/
void IO_printAdc(uint32_t x){

  int v;
  char stemc[LENGTH_PRINT_ADC]="";
  v = (int)(sizeof(stemc) * (float)x/4095.0); // escalar y convertir a entero
  if (v>sizeof(stemc)-1) v=sizeof(stemc)-1;  // asegurar que v no supere el largo de stemc
  memset(stemc,'*', v);
  stemc[v]='\0';
  printf("%s\n", stemc);

}

/********************************** GPIO **************************************/

/******************************************************************************
IO_gpioInit(): inicializa perifericos de entrada/salida
*******************************************************************************/
void IO_gpioInit(){

  /* Configure the IOMUX register for pad BLINK_GPIO (some pads are muxed to GPIO
     on reset already, but some default to other functions and need to be switched
     to GPIO. Consult the Technical Reference for a list of pads and their default
     functions.) */

  gpio_reset_pin(BLINK_GPIO);
  /* Set the GPIO as a push/pull output */
  gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
  ESP_LOGI(TAG, "GPIO %d MODE %d ",BLINK_GPIO , GPIO_MODE_OUTPUT );
  gpio_set_level(BLINK_GPIO, 0);

}

/******************************************************************************
 IO_setLed(): Setea el estado del LED configurado por defecto en el módulo ESP32
******************************************************************************/
int IO_setLed(int estado){

  gpio_set_level(BLINK_GPIO, estado);
  return estado;

}

/******************************************************************************
 IO_toggleLed(): Togglea estado del LED configurado por defecto en el módulo ESP32
******************************************************************************/
void IO_toggleLed(void){

 static int estado = 0;
 estado = 1 - estado;
 gpio_set_level(BLINK_GPIO, estado);

}
