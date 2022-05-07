/** \file	sd.c
 *  Mar 2022
 *  Maestría en Sistemas Embebidos - Sistemas embebidos distribuidos
 * \brief Contiene las funciones para la manipulación de archivos en memoria SD (SPI)
 */

 #include <stdio.h>
 #include <string.h>
 #include <sys/unistd.h>
 #include <sys/stat.h>
 #include "esp_err.h"
 #include "esp_log.h"
 #include "esp_vfs_fat.h"
 #include "driver/sdspi_host.h"
 #include "driver/spi_common.h"
 #include "sdmmc_cmd.h"
 #include "sdkconfig.h"
 #include <sys/time.h>

 #ifdef CONFIG_IDF_TARGET_ESP32
 #include "driver/sdmmc_host.h"
 #endif


/* APID */
#include "config.h"
#include "sd.h"

/* TAGs */
static const char *TAG = "SD";

/********************************* SD *****************************************/

// Definiciones

#define MOUNT_POINT "/sdcard"

// Habilitamos el modo SPI como interfaz para el manejo de la SD:
#define USE_SPI_MODE

// ESP32-S2 and ESP32-C3 doesn't have an SD Host peripheral, always use SPI:
#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32C3
// on ESP32-S2, DMA channel must be the same as host id
#if CONFIG_IDF_TARGET_ESP32S2
#define SPI_DMA_CHAN    host.slot
#elif CONFIG_IDF_TARGET_ESP32C3
#define SPI_DMA_CHAN    SPI_DMA_CH_AUTO
#endif //CONFIG_IDF_TARGET_ESP32S2_
#endif //CONFIG_IDF_TARGET_ESP32S2

// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
#define SPI_DMA_CHAN    1
#endif //SPI_DMA_CHAN

// Pin mapping when using SPI mode.
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2

#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 15
#define PIN_NUM_CLK  14
#define PIN_NUM_CS   13

#elif CONFIG_IDF_TARGET_ESP32C3
#define PIN_NUM_MISO 18
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK  8
#define PIN_NUM_CS   19

#endif //CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2

#define MAX_NAMEDIR_LENGTH 200
#define MAX_DATA_LENGTH 1000

char buffer[MAX_DATA_LENGTH]; // data buffer
sdmmc_card_t *card;
const char mount_point[] = MOUNT_POINT;
sdmmc_host_t host = SDSPI_HOST_DEFAULT();

/* FUNCIONES DEFINIDAS EN EL NIVEL DE ABSTRACCION SD
SD_open(): abrir archivo
SD_close(): cerrar archivo
SD_write(): escribir datos (bytes)
SD_read(): leer datos (bytes)
SD_printf(): escribir texto con formato
SD_gets(): leer una linea de texto
SD_puts(): Escribir una línea de texto
SD_setPos(): mover la posición del puntero de lectura/escritura de un archivo
SD_getPos(): leer la posición del puntero de lectura/escritura de un archivo
SD_getSize(): devuelve el tamaño del archivo
SD_mkdir(): crear un directorio
SD_rename(): renombrar archivo
SD_remove(): eliminar carpeta o archivo
SD_mount(): montar la unidad de momoria SD
SD_unmount(): desmontar la unidad de memoria SD
*/


/******************************************************************************
SD_open(): abrir un archivo cuya ruta/nombre se especifica en filename.
EL parámetro "option" permite elegir el modo de operación (escritura "w", lectura "r", etc).
Retorna un puntero a FILE si se abrió correctamente. Sino, retorna NULL.
*******************************************************************************/
FILE * SD_open( const char * filename, const char * option){

    // Use POSIX and C standard library functions to work with files.
    FILE * f;
    ESP_LOGI(TAG, "Opening file");
    char filedir[MAX_NAMEDIR_LENGTH]=MOUNT_POINT"/";
    strcat(filedir, filename);

    f = fopen(filedir, option);

    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file");
        return NULL;
    }

   return f;
}

/******************************************************************************
SD_close(): Cierra archivo especificado en f.
********************************************************************************/
int SD_close(FILE * f){

  return fclose(f);

}

/******************************************************************************
SD_write(): Escribe los datos ingresados en el array "bytes" de largo "nbytes".
********************************************************************************/
int SD_write(FILE * f, uint8_t * bytes, int nbytes){

  return fwrite(bytes, 1, nbytes, f);

}

/******************************************************************************
SD_read(): lee los datos ingresados en "bytes", para un tamaño de "nbytes".
******************************************************************************/
int SD_read(FILE * f,  uint8_t * bytes, uint32_t nbytes){

    return fread(bytes, nbytes, 1, f);

}

/******************************************************************************
SD_printf(): Es identica a fprintf(). Se redefine el nombre para que sea compatible
con la notación del nivel de abstracción.
********************************************************************************/
// #define  SD_printf fprintf // MACRO definida en sd.h
// SD_printf(f,....)


/******************************************************************************
SD_gets(): lee una línea del archivo especificado en f. El texto se pasa por
referencia en "line" y debe especificarse en "nmax" la máxima cantidad de caracteres
a leer. Si se intenta leer más allá de la última línea, devuelve 0.
Si la lectura fue exitosa retorna 1.
******************************************************************************/
int SD_gets(FILE * f,  char * line, int nmax){

  if(fgets(line, nmax, f)!=NULL){
    // anulamos el '\n' que fgets() agrega al final de la cadena.
    int n = strlen(line);
    if (line[n-1] == '\n'){
    line[n-1]='\0'; // comentar esta línea si se quiere incluir '\n'
    }
    return 1;
  }
  else{
    ESP_LOGI(TAG, "NULL: se superó la última línea\n");
    return 0;
  }

}


/******************************************************************************
SD_puts(): Escribe una línea de texto de la cadena ingresada en "line" sin
incluir el caracter nulo. Si la escritura resulta exitosa devuelve un valor no nulo.
Si se alcanzó o superó EOF, devuelve NULL.
********************************************************************************/
int SD_puts(FILE * f, const char * line){

  return fputs(line, f);

}

/******************************************************************************
SD_setPos(): Ubica el puntero actual del archivo f en la posición especificada
(en nro de bytes) en "new_pos". Retorna el valor de posición anterior
********************************************************************************/
uint32_t SD_setPos(FILE * f, uint32_t new_pos){
  /* fseeko(f, off64_t offset, int whence)
  whence:
  1	SEEK_SET: Beginning of file
  2	SEEK_CUR: Current position of the file pointer
  3	SEEK_END: End of file */

  // guardamos el offset actual
   uint32_t old_pos =  ftello(f);

  // por defecto se considera el offset desde el inicio del archivo: SEEK_SET
  fseeko(f, new_pos, SEEK_SET);

  return old_pos;
}

/******************************************************************************
SD_getPos(): obtiene la posición (nro de bytes) del puntero actual en el arcivo f
********************************************************************************/
uint32_t SD_getPos(FILE * f){

  return (uint32_t) ftello(f);

}


/******************************************************************************
SD_getSize(): Devuelve el tamaño actual del archivo f
********************************************************************************/
uint32_t SD_getSize(FILE * f){

  struct stat st;
  int fd = fileno(f);
  fstat(fd, &st);

  return (uint32_t)st.st_size;

}

/******************************************************************************
 SD_mkdir(): Crea una carpeta cuyo nombre se especifica en "dirname".
 No se puede crear una ruta a un subdirectorio de una sola vez. Por ejemplo,
 para crear "carpeta/carpeta1/carpeta2":
   SD_mkdir("carpeta");
   SD_mkdir("carpeta/carpeta1");
   SD_mkdir("carpeta/carpeta1/carpeta2");
 Retorna: 1 si se creo exitosamente, 0 si ya existe, -1 si falló la operación.
*******************************************************************************/
int SD_mkdir(const char * dir_name){

  char dir[MAX_NAMEDIR_LENGTH] = MOUNT_POINT"/";
  struct stat st;
  strcat(dir, dir_name);

  if (stat( dir, &st) == 0) {
    ESP_LOGE(TAG, "The specified folder already exists");
    return -1;
  }

  int ret = mkdir(dir, 0777);

  if (ret == -1){
    ESP_LOGE(TAG, "Failed to make folder");
    return 0;
  }

  else {
    ESP_LOGI(TAG, "Created folder: %s", dir);
    return 1;
  }

}

/******************************************************************************
  SD_rename(): Modifica el nombre de un archivo o carpeta. Si el nuevo nombre
  ya existe anula la operación y devuelve 0. Si el nombre está disponible,
  cambia el nombre especificado y devuelve 1. En caso que falle el cambio de nombre
  retorna con -1.
*******************************************************************************/
int SD_rename(const char * old_name, const char * new_name){

  // Check if destination file exists before renaming
  struct stat st;
  char old[MAX_NAMEDIR_LENGTH]=MOUNT_POINT"/";
  char new[MAX_NAMEDIR_LENGTH]=MOUNT_POINT"/";
  strcat(old, old_name);
  strcat(new, new_name);

  if (stat( new, &st) == 0) {
    // Si el nomnre ya existe anula la operación
    ESP_LOGE(TAG, "The new name already exists. Choose another name!");
    return 0;
  }

  // Rename original file
  ESP_LOGI(TAG, "Renaming file %s to %s ...",  old, new);

  if (rename( old,  new) == 0) {
      ESP_LOGI(TAG, "Name changed successfully!");
      return 1;
    }

  else {
      ESP_LOGE(TAG, "Rename failed");
      return -1;
  }

}


/******************************************************************************
 SD_remove(): Borra carpeta o archivo especificado en "dirname".
 Si se borró exitosamente retorna 1. Si no existe, no hace nada y retorna 0.
******************************************************************************/
int SD_remove(const char * dirname){

  struct stat st;

  char dir[MAX_NAMEDIR_LENGTH] = MOUNT_POINT"/";

  strcat(dir, dirname);

  if (stat(dir, &st) == 0) {
    // Delete it if it exists
    unlink(dir);
    ESP_LOGI(TAG, "The folder or file was successfully deleted!");
    return 1;
  }
  else {

    ESP_LOGE(TAG, "The folder or file does not exist!");
    return 0;
  }

}


/*******************************************************************************
 SD_mount(): Configuración del modo SPI para lectura/escritura de SD y montaje de la unidad
****************************************************************************** */
void SD_mount(){

  esp_err_t ret;

  // Options for mounting the filesystem.
  // If format_if_mount_failed is set to true, SD card will be partitioned and
  // formatted in case when mounting fails.
  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    #ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .format_if_mount_failed = true,
    #else
    .format_if_mount_failed = false,
    #endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
    .max_files = 5,
    .allocation_unit_size = 16 * 1024
  };

  ESP_LOGI(TAG, "Initializing SD card");

  // Use settings defined above to initialize SD card and mount FAT filesystem.
  // Please check its source code and implement error recovery when developing
  // production applications.
  ESP_LOGI(TAG, "Using SPI peripheral");

  spi_bus_config_t bus_cfg = {
    .mosi_io_num = PIN_NUM_MOSI,
    .miso_io_num = PIN_NUM_MISO,
    .sclk_io_num = PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 4000,
  };

  ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize bus.");
    return;
  }

  // This initializes the slot without card detect (CD) and write protect (WP) signals.
  // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = PIN_NUM_CS;
  slot_config.host_id = host.slot;


  ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
      "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    } else {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
      "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
    }
    return;
  }
  ESP_LOGI(TAG, "Filesystem mounted");

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card);

}


/******************************************************************************
SD_unmount(): desmonta la unidad de tarjeta SD montada en la inicialización
******************************************************************************/
void SD_unmount(){

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");
    //deinitialize the bus after all devices are removed
    spi_bus_free(host.slot);

}
