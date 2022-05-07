/** \file	main.h
 *  Mar 2022
 *  Maestría en SIstemas Embebidos / Sistemas embebidos distribuidos
 *  \brief	Contiene las bibliotecas y definiciones generales de configuración
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "sdkconfig.h"

/* Configuración general  */
#include "config.h"

/* Nivel de abstracción APID */
#include "mqtt.h"
#include "wifi.h"
#include "io.h"
#include "crono.h"
#include "sd.h"

void compareThreshold(void);


#endif /* MAIN_H_ */
