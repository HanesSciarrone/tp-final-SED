/** \file sd.h
 *  Mar 2022
 *  Maestría en SIstemas Embebidos - Sistemas embebidos distribuidos
 */


#ifndef SD_H_
#define SD_H_

#include <stdio.h>

FILE * SD_open( const char * , const char * );
int SD_close(FILE * );
int SD_write(FILE * , uint8_t * , int );
int SD_read(FILE * ,  uint8_t * , uint32_t );
#define  SD_printf fprintf
int SD_gets(FILE * ,  char * , int );
int SD_puts(FILE * , const char * );
uint32_t SD_setPos(FILE * , uint32_t );
uint32_t SD_getPos(FILE * );
uint32_t SD_getSize(FILE * );
int SD_mkdir(const char * );
int SD_rename(const char * , const char * );
int SD_remove(const char * );
void SD_mount();
void SD_unmount();

#endif
